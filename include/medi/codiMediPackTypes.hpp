/*
 * MeDiPack, a Message Differentiation Package
 *
 * Copyright (C) 2017 Chair for Scientific Computing (SciComp), TU Kaiserslautern
 * Homepage: http://www.scicomp.uni-kl.de
 * Contact:  Prof. Nicolas R. Gauger (codi@scicomp.uni-kl.de)
 *
 * Lead developers: Max Sagebaum (SciComp, TU Kaiserslautern)
 *
 * This file is part of MeDiPack (http://www.scicomp.uni-kl.de/software/codi).
 *
 * MeDiPack is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * MeDiPack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU
 * General Public License along with MeDiPack.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Max Sagebaum (SciComp, TU Kaiserslautern)
 */

#pragma once

#include "ampi/ampiMisc.h"

#include "adjointInterface.hpp"
#include "adToolInterface.h"
#include "ampi/typeDefault.hpp"
#include "adToolImplCommon.hpp"
#include "ampi/op.hpp"
#include "ampi/types/indexTypeHelper.hpp"

#include <adjointInterface.hpp>

template<typename CoDiType>
struct CoDiMeDiAdjointInterfaceWrapper : public medi::AdjointInterface {

    typedef typename CoDiType::Real Real;
    typedef typename CoDiType::GradientData IndexType;

    codi::AdjointInterface<Real>* codiInterface;

    int vecSize;

    CoDiMeDiAdjointInterfaceWrapper(codi::AdjointInterface<Real>* interface) :
      codiInterface(interface),
      vecSize((int)interface->getVectorSize()) {}

    int computeElements(int elements) const {
      return elements * vecSize;
    }

    int getVectorSize() const {
      return vecSize;
    }

    inline void getAdjoints(const void* i, void* a, int elements) const {
      Real* adjoints = (Real*)a;
      IndexType* indices = (IndexType*)i;

      for(int pos = 0; pos < elements; ++pos) {
        codiInterface->getAdjointVec(indices[pos], &adjoints[pos * vecSize]);
        codiInterface->resetAdjointVec(indices[pos]);
      }
    }

    inline void updateAdjoints(const void* i, const void* a, int elements) const {
      Real* adjoints = (Real*)a;
      IndexType* indices = (IndexType*)i;

      for(int pos = 0; pos < elements; ++pos) {

        codiInterface->updateAdjointVec(indices[pos], &adjoints[pos * vecSize]);
      }
    }

    inline void setReverseValues(const void* i, const void* p, int elements) const {
      Real* primals = (Real*)p;
      IndexType* indices = (IndexType*)i;

      for(int pos = 0; pos < elements; ++pos) {
        codiInterface->resetPrimal(indices[pos], primals[pos]);
      }
    }

    inline void combineAdjoints(void* b, const int elements, const int ranks) const {
      Real* buf = (Real*)b;

      for(int curRank = 1; curRank < ranks; ++curRank) {
        for(int curPos = 0; curPos < elements; ++curPos) {
          for(int dim = 0; dim < vecSize; ++dim) {

            buf[curPos * vecSize + dim] += buf[(elements * curRank + curPos) * vecSize + dim];
          }
        }
      }
    }

    inline void createAdjointTypeBuffer(void* &buf, size_t size) const {
      buf = (void*)(new Real[size * vecSize]);
    }

    inline void deleteAdjointTypeBuffer(void* &b) const {
      if(NULL != b) {
        Real* buf = (Real*)b;
        delete [] buf;
        b = NULL;
      }
    }
};



template<typename CoDiType, bool primalRestore, typename Impl>
struct CoDiPackToolBase : public medi::ADToolImplCommon<Impl, primalRestore, false, CoDiType, typename CoDiType::GradientValue, typename CoDiType::PassiveReal, typename CoDiType::GradientData> {
  typedef CoDiType Type;
  typedef void AdjointType;
  typedef CoDiType ModifiedType;
  typedef typename CoDiType::PassiveReal PassiveType;
  typedef typename CoDiType::GradientData IndexType;

  typedef typename CoDiType::TapeType Tape;

  static MPI_Datatype MpiType;
  static MPI_Datatype ModifiedMpiType;
  static MPI_Datatype AdjointMpiType;

  typedef medi::MpiTypeDefault<Impl> MediType;
  static MediType* MPI_TYPE;
  static medi::AMPI_Datatype MPI_INT_TYPE;

  static medi::OperatorHelper<
            medi::FunctionHelper<
                CoDiType, CoDiType, typename CoDiType::PassiveReal, typename CoDiType::GradientData, typename CoDiType::GradientValue, Impl
            >
          > operatorHelper;

  static Tape* adjointTape;

  CoDiPackToolBase(MPI_Datatype adjointMpiType) :
    medi::ADToolImplCommon<Impl, primalRestore, false, CoDiType, typename CoDiType::GradientValue, typename CoDiType::PassiveReal, typename CoDiType::GradientData>(adjointMpiType) {}

  static void initTypes() {
    // create the mpi type for CoDiPack
    // this type is used in this type and the passive formulation
    // TODO: add proper type creation
    MPI_Type_contiguous(sizeof(CoDiType), MPI_BYTE, &MpiType);
    MPI_Type_commit(&MpiType);

    ModifiedMpiType = MpiType;

    // Since we use the CoDiPack adjoint interface, everything is interpreted in terms of the primal computation type
    // TODO: add proper type creation
    MPI_Type_contiguous(sizeof(typename CoDiType::Real), MPI_BYTE, &AdjointMpiType);
    MPI_Type_commit(&AdjointMpiType);
  }

  static void init() {
    initTypes();

    MPI_TYPE = new MediType();

    operatorHelper.init(MPI_TYPE);
    MPI_INT_TYPE = operatorHelper.MPI_INT_TYPE;
  }

  static void finalizeTypes() {
    MPI_Type_free(&MpiType);
  }

  static void finalize() {

    operatorHelper.finalize();

    if(nullptr != MPI_TYPE) {
      delete MPI_TYPE;
      MPI_TYPE = nullptr;
    }

    finalizeTypes();
  }

  inline  bool isHandleRequired() const {
    return Type::getGlobalTape().isActive();
  }

  inline void startAssembly(medi::HandleBase* h) const {
    MEDI_UNUSED(h);

  }

  inline void addToolAction(medi::HandleBase* h) const {
    if(NULL != h) {
      Type::getGlobalTape().pushExternalFunctionHandle(callFunc, h, deleteFunc);
    }
  }

  medi::AMPI_Op convertOperator(medi::AMPI_Op op) const {
    return operatorHelper.convertOperator(op);
  }

  inline void stopAssembly(medi::HandleBase* h) const {
    MEDI_UNUSED(h);
  }

  static void callFunc(void* tape, void* h, void* ah) {
    adjointTape = (Tape*)tape;
    medi::HandleBase* handle = static_cast<medi::HandleBase*>(h);
    CoDiMeDiAdjointInterfaceWrapper<CoDiType> ahWrapper((codi::AdjointInterface<typename CoDiType::Real>*)ah);
    handle->func(handle, &ahWrapper);
  }

  static void deleteFunc(void* tape, void* h) {
    MEDI_UNUSED(tape);

    medi::HandleBase* handle = static_cast<medi::HandleBase*>(h);
    delete handle;
  }

  static inline IndexType getIndex(const Type& value) {
    return value.getGradientData();
  }

  static inline void clearIndex(Type& value) {
    IndexType oldIndex = value.getGradientData();
    value.~Type();
    value.getGradientData() = oldIndex;  // restore the index here so that the other side can decide of the communication was active or not
  }

  static inline PassiveType getValue(const Type& value) {
    return value.getValue();
  }

  static inline void setIntoModifyBuffer(ModifiedType& modValue, const Type& value) {
    MEDI_UNUSED(modValue);
    MEDI_UNUSED(value);
  }

  static inline void getFromModifyBuffer(const ModifiedType& modValue, Type& value) {
    MEDI_UNUSED(modValue);
    if(0 != value.getGradientData()) {
      value.getGradientData() = IndexType();
      Type::getGlobalTape().registerInput(value);
    }
  }

  static PassiveType getPrimalFromMod(const ModifiedType& modValue) {
    return modValue.value();
  }

  static void setPrimalToMod(ModifiedType& modValue, const PassiveType& value) {
    modValue.value() = value;
  }

  static void modifyDependency(ModifiedType& inval, ModifiedType& inoutval) {

    bool active = (0 != inoutval.getGradientData()) | (0 != inval.getGradientData());
    if(active) {
      inoutval.getGradientData() = -1; // TODO: Define invalid index in CoDiPack
    } else {
      inoutval.getGradientData() = 0; // TODO: Define passive index in CoDiPack
    }
  }
};

template<typename CoDiType>
struct CoDiPackTool final : public CoDiPackToolBase<CoDiType, false, CoDiPackTool<CoDiType> >  {

    typedef CoDiType Type;
    typedef typename CoDiType::TapeType Tape;
    typedef void AdjointType;
    typedef CoDiType ModifiedType;
    typedef typename CoDiType::PassiveReal PassiveType;
    typedef typename CoDiType::GradientData IndexType;

    CoDiPackTool(MPI_Datatype adjointMpiType) :
      CoDiPackToolBase<CoDiType, false, CoDiPackTool< CoDiType>>(adjointMpiType) {}

    static inline IndexType registerValue(Type& value, PassiveType& oldPrimal) {
      MEDI_UNUSED(oldPrimal);

      bool wasActive = 0 != value.getGradientData();
      value.getGradientData() = IndexType();

      // make the value active again if it has been active before on the other processor
      if(wasActive) {
        Type::getGlobalTape().registerInput(value);
      }

      return value.getGradientData();
    }
};

template<typename CoDiType>
struct CoDiPackToolPrimalRestore final : public CoDiPackToolBase<CoDiType, true, CoDiPackToolPrimalRestore<CoDiType> >  {

    typedef CoDiType Type;
    typedef typename CoDiType::TapeType Tape;
    typedef void AdjointType;
    typedef CoDiType ModifiedType;
    typedef typename CoDiType::PassiveReal PassiveType;
    typedef typename CoDiType::GradientData IndexType;

    CoDiPackToolPrimalRestore(MPI_Datatype adjointMpiType) :
      CoDiPackToolBase<CoDiType, true, CoDiPackToolPrimalRestore< CoDiType>>(adjointMpiType) {}

    static inline IndexType registerValue(Type& value, PassiveType& oldPrimal) {
      bool wasActive = 0 != value.getGradientData();
      value.getGradientData() = IndexType();

      if(wasActive) {
        oldPrimal = Type::getGlobalTape().registerExtFunctionOutput(value);
      } else {
        oldPrimal = 0.0;
      }

      return value.getGradientData();
    }
};

template<typename CoDiType, bool primalRestore, typename Impl> MPI_Datatype CoDiPackToolBase<CoDiType, primalRestore, Impl>::MpiType;
template<typename CoDiType, bool primalRestore, typename Impl> MPI_Datatype CoDiPackToolBase<CoDiType, primalRestore, Impl>::ModifiedMpiType;
template<typename CoDiType, bool primalRestore, typename Impl> MPI_Datatype CoDiPackToolBase<CoDiType, primalRestore, Impl>::AdjointMpiType;
template<typename CoDiType, bool primalRestore, typename Impl> typename CoDiPackToolBase<CoDiType, primalRestore, Impl>::MediType* CoDiPackToolBase<CoDiType, primalRestore, Impl>::MPI_TYPE;
template<typename CoDiType, bool primalRestore, typename Impl> medi::AMPI_Datatype CoDiPackToolBase<CoDiType, primalRestore, Impl>::MPI_INT_TYPE;
template<typename CoDiType, bool primalRestore, typename Impl> medi::OperatorHelper<medi::FunctionHelper<CoDiType, CoDiType, typename CoDiType::PassiveReal, typename CoDiType::GradientData, typename CoDiType::GradientValue, Impl> > CoDiPackToolBase<CoDiType, primalRestore, Impl>::operatorHelper;
template<typename CoDiType, bool primalRestore, typename Impl> typename CoDiType::TapeType* CoDiPackToolBase<CoDiType, primalRestore, Impl>::adjointTape;
