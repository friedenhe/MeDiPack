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

#include "adToolInterface.h"
#include "mpiTypeDefault.hpp"
#include "ampi/types/indexTypeHelper.hpp"

template <typename T>
void codiUnmodifiedAdd(T* invec, T* inoutvec, int* len, MPI_Datatype* datatype) {
  MEDI_UNUSED(datatype);

  for(int i = 0; i < *len; ++i) {
    inoutvec[i] += invec[i];
  }
}

template <typename T>
void codiUnmodifiedMul(T* invec, T* inoutvec, int* len, MPI_Datatype* datatype) {
  MEDI_UNUSED(datatype);

  for(int i = 0; i < *len; ++i) {
    inoutvec[i] *= invec[i];
  }
}

template <typename T>
void codiUnmodifiedMax(T* invec, T* inoutvec, int* len, MPI_Datatype* datatype) {
  MEDI_UNUSED(datatype);

  using std::max;
  for(int i = 0; i < *len; ++i) {
    inoutvec[i] = max(inoutvec[i], invec[i]);
  }
}

template <typename T>
void codiUnmodifiedMin(T* invec, T* inoutvec, int* len, MPI_Datatype* datatype) {
  MEDI_UNUSED(datatype);

  using std::min;
  for(int i = 0; i < *len; ++i) {
    inoutvec[i] = min(inoutvec[i], invec[i]);
  }
}

template<typename CoDiType>
struct CoDiPackForwardTool final : public medi::ADToolBase<CoDiPackForwardTool<CoDiType>, typename CoDiType::GradientValue, typename CoDiType::PassiveReal, int> {
  typedef CoDiType Type;
  typedef typename CoDiType::GradientValue AdjointType;
  typedef CoDiType ModifiedType;
  typedef typename CoDiType::PassiveReal PassiveType;
  typedef int IndexType;

  const static bool IS_ActiveType = false;
  const static bool IS_RequiresModifiedBuffer = false;

  static MPI_Datatype MpiType;
  static MPI_Datatype ModifiedMpiType;
  static MPI_Datatype AdjointMpiType;

  static medi::AMPI_Op OP_SUM;
  static medi::AMPI_Op OP_PROD;
  static medi::AMPI_Op OP_MIN;
  static medi::AMPI_Op OP_MAX;
  static medi::AMPI_Op OP_MINLOC;
  static medi::AMPI_Op OP_MAXLOC;

  typedef medi::MpiTypeDefault<CoDiPackForwardTool> MediType;
  static MediType* MPI_TYPE;
  static medi::AMPI_Datatype MPI_INT_TYPE;

  static medi::OperatorHelper<
            medi::FunctionHelper<
                CoDiType, CoDiType, typename CoDiType::PassiveReal, typename CoDiType::GradientData, typename CoDiType::GradientValue, CoDiPackForwardTool<CoDiType>
            >
          > operatorHelper;

  static void initTypes() {
    // create the mpi type for CoDiPack
    // this type is used in this type and the passive formulation
    MPI_Type_contiguous(sizeof(CoDiType), MPI_BYTE, &MpiType);
    MPI_Type_commit(&MpiType);

    ModifiedMpiType = MpiType;
    AdjointMpiType = MPI_DOUBLE;
  }

  static void init() {
    initTypes();
    initOperators();

    MPI_TYPE = new MediType();

    operatorHelper.init(MPI_TYPE);
    MPI_INT_TYPE = operatorHelper.MPI_INT_TYPE;

    OP_SUM = operatorHelper.OP_SUM;
    OP_PROD = operatorHelper.OP_PROD;
    OP_MIN = operatorHelper.OP_MIN;
    OP_MAX = operatorHelper.OP_MAX;
    OP_MINLOC = operatorHelper.OP_MINLOC;
    OP_MAXLOC = operatorHelper.OP_MAXLOC;
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

  CoDiPackForwardTool(MPI_Datatype adjointMpiType) :
    medi::ADToolBase<CoDiPackForwardTool<CoDiType>, typename CoDiType::GradientValue, typename CoDiType::PassiveReal, int>(adjointMpiType) {}


  inline bool isActiveType() const {
    return false;
  }

  inline  bool isHandleRequired() const {
    return false;
  }

  inline bool isOldPrimalsRequired() const {
    return false;
  }

  inline void startAssembly(medi::HandleBase* h) {
    MEDI_UNUSED(h);

  }

  inline void addToolAction(medi::HandleBase* h) {
    MEDI_UNUSED(h);
  }

  inline void stopAssembly(medi::HandleBase* h) {
    MEDI_UNUSED(h);
  }

  medi::AMPI_Op convertOperator(medi::AMPI_Op op) const {
    return operatorHelper.convertOperator(op);
  }

  inline void getAdjoints(const IndexType* indices, AdjointType* adjoints, int elements) const {
    MEDI_UNUSED(indices);
    MEDI_UNUSED(adjoints);
    MEDI_UNUSED(elements);
  }

  inline void updateAdjoints(const IndexType* indices, const AdjointType* adjoints, int elements) const {
    MEDI_UNUSED(indices);
    MEDI_UNUSED(adjoints);
    MEDI_UNUSED(elements);
  }

  inline void setReverseValues(const IndexType* indices, const PassiveType* primals, int elements) const {
    MEDI_UNUSED(indices);
    MEDI_UNUSED(primals);
    MEDI_UNUSED(elements);

    /* not required */
  }

  inline void combineAdjoints(AdjointType* buf, const int elements, const int ranks) const {
    MEDI_UNUSED(buf);
    MEDI_UNUSED(elements);
    MEDI_UNUSED(ranks);
  }

  inline void createAdjointTypeBuffer(AdjointType* &buf, size_t size) const {
    buf = new AdjointType[size];
  }

  inline void createPassiveTypeBuffer(PassiveType* &buf, size_t size) const {
    buf = new PassiveType[size];
  }

  inline void createIndexTypeBuffer(IndexType* &buf, size_t size) const {
    buf = new IndexType[size];
  }

  inline void deleteAdjointTypeBuffer(AdjointType* &buf) const {
    if(NULL != buf) {
      delete [] buf;
      buf = NULL;
    }
  }

  inline void deletePassiveTypeBuffer(PassiveType* &buf) const {
    if(NULL != buf) {
      delete [] buf;
      buf = NULL;
    }
  }

  inline void deleteIndexTypeBuffer(IndexType* &buf) const {
    if(NULL != buf) {
      delete [] buf;
      buf = NULL;
    }
  }

  static void callFunc(void* h) {
    medi::HandleBase* handle = static_cast<medi::HandleBase*>(h);
    handle->func(handle);
  }

  static void deleteFunc(void* h) {
    medi::HandleBase* handle = static_cast<medi::HandleBase*>(h);
    delete handle;
  }

  static inline int getIndex(const Type& value) {
    return value.getGradientData();
  }

  static inline void clearIndex(Type& value) {
    value.~Type();
    value.getGradientData() = 0;
  }

  static inline PassiveType getValue(const Type& value) {
    return value.getValue();
  }

  static inline void setValue(const IndexType& index, const PassiveType& primal) {
    MEDI_UNUSED(index);
    MEDI_UNUSED(primal);
  }

  static inline void setIntoModifyBuffer(ModifiedType& modValue, const Type& value) {
    MEDI_UNUSED(modValue);
    MEDI_UNUSED(value);
  }

  static inline void getFromModifyBuffer(const ModifiedType& modValue, Type& value) {
    MEDI_UNUSED(modValue);
    MEDI_UNUSED(modValue);
  }

  static inline int registerValue(Type& value, PassiveType& oldValue) {
    MEDI_UNUSED(value);
    MEDI_UNUSED(oldValue);
    return 0;
  }

  static bool isActive() {
    return false;
  }

  static PassiveType getPrimalFromMod(const ModifiedType& modValue) {
    return modValue.value();
  }

  static void setPrimalToMod(ModifiedType& modValue, const PassiveType& value) {
    modValue.value() = value;
  }

  static void modifyDependency(ModifiedType& inval, ModifiedType& inoutval) {
    MEDI_UNUSED(inval);
    MEDI_UNUSED(inoutval);
  }
};

template<typename CoDiType> MPI_Datatype CoDiPackForwardTool<CoDiType>::MpiType;
template<typename CoDiType> MPI_Datatype CoDiPackForwardTool<CoDiType>::ModifiedMpiType;
template<typename CoDiType> MPI_Datatype CoDiPackForwardTool<CoDiType>::AdjointMpiType;
template<typename CoDiType> medi::AMPI_Op CoDiPackForwardTool<CoDiType>::OP_SUM;
template<typename CoDiType> medi::AMPI_Op CoDiPackForwardTool<CoDiType>::OP_PROD;
template<typename CoDiType> medi::AMPI_Op CoDiPackForwardTool<CoDiType>::OP_MIN;
template<typename CoDiType> medi::AMPI_Op CoDiPackForwardTool<CoDiType>::OP_MAX;
template<typename CoDiType> medi::AMPI_Op CoDiPackForwardTool<CoDiType>::OP_MINLOC;
template<typename CoDiType> medi::AMPI_Op CoDiPackForwardTool<CoDiType>::OP_MAXLOC;
template<typename CoDiType> typename CoDiPackForwardTool<CoDiType>::MediType* CoDiPackForwardTool<CoDiType>::MPI_TYPE;
template<typename CoDiType> typename CoDiPackForwardTool<CoDiType>::MediType* CoDiPackForwardTool<CoDiType>::MPI_INT_TYPE;
template<typename CoDiType> medi::OperatorHelper<medi::FunctionHelper<CoDiType, CoDiType, typename CoDiType::PassiveReal, typename CoDiType::GradientData, typename CoDiType::GradientValue, CoDiPackForwardTool<CoDiType> > CoDiPackForwardTool<CoDiType>::operatorHelper;
