/*
 * Copyright (c) 2015-2022 Simple Device Model contributors
 * 
 * This file is part of the Simple Device Model (SDM) framework.
 * 
 * SDM framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SDM framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SDM framework.  If not, see <https://www.gnu.org/licenses/>.
 *
 * DFIC (Dynamic Function Interface Compiler) is a simple FFI
 * (foreign function interface) implementation. It supports
 * calling external C functions, with some limitations:
 *
 *   * Arguments and return values must be of arithmetic
 *     or pointer types (pointers to members are not supported)
 *
 *   * Supported argument sizes are 1, 2, 4 and 8 bytes
 *
 * DFIC works by compiling function wrappers at run time. It
 * supports x86 and x86_64 architectures.
 * 
 * This header file defines DFIC public interface.
 */

#ifndef DFIC_H_INCLUDED
#define DFIC_H_INCLUDED

#ifdef _MSC_VER
	#pragma warning(disable: 4800) // disable warning about casting to bool
#endif

#include <vector>
#include <string>
#include <cstdint>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <cstring>

#if defined(__i386) || defined(_M_IX86)
	#define DFIC_CPU_X86
#elif defined(__x86_64__) || defined(_M_X64)
	#define DFIC_CPU_X64
#else
	#error "Unsupported CPU: x86 or x86_64 required"
#endif

namespace Dfic {
	typedef void(*GenericFuncPtr)();
	typedef std::uintptr_t MachineWord;
	
	enum CallingConvention {CDecl,Pascal,StdCall,FastCall,ThisCall,X64Native};
	
// Note: Standard C++ doesn't allow casts between function and data pointers
	template <typename T> GenericFuncPtr toFuncPtr(const T *p) {
		static_assert(sizeof(GenericFuncPtr)==sizeof(T*),"Function and object pointer sizes differ");
		GenericFuncPtr res;
		std::memcpy(&res,&p,sizeof(GenericFuncPtr));
		return res;
	}
	
	template <typename R,typename... Params> void *toDataPtr(R(*func)(Params...)) {
		static_assert(sizeof(void *)==sizeof(R(*)(Params...)),"Function and object pointer sizes differ");
		void *res;
		std::memcpy(&res,&func,sizeof(void *));
		return res;
	}
	
	struct Argument final {
		enum Traits {UnsignedIntegral,SignedIntegral,Float,Pointer};
		
		std::vector<MachineWord> buf;
		std::size_t typeSize;
		Traits traits;
		
		Argument(std::size_t s,Traits t):
			buf(s%sizeof(MachineWord)?s/sizeof(MachineWord)+1:s/sizeof(MachineWord)),
			typeSize(s),
			traits(t) {}
	};
	
	class Interface final {
		enum BufferAccessMode {None,ReadWrite,Execute};
		
		std::vector<std::unique_ptr<Argument> > _args;
		GenericFuncPtr _func;
		
		CallingConvention _cc;
		
		std::uint64_t _retInt;
		float _retFloat;
		double _retDouble;

// Metadata (not to be copied)
		char *_code=nullptr;
		char *_currentCode;
		BufferAccessMode _codeAccess=None;
		std::size_t _codeBufSize;
		bool _dirty=true;
	
	public:
		Interface();
		template <typename F> explicit Interface(const F &f): Interface() {
			setFunctionPointer<F>(f);
		}
		Interface(const Interface &);
		Interface(Interface &&);
		~Interface();
		
		Interface &operator=(const Interface &);
		Interface &operator=(Interface &&);
		
		void reset();

// Underlying function pointer
		GenericFuncPtr functionPointer() const {return _func;}
		template <typename F> void setFunctionPointer(const F &f) {
			_func=reinterpret_cast<GenericFuncPtr>(f);
			_dirty=true;
		}

// Calling convention (makes sense for x86)
		CallingConvention callingConvention() const {return _cc;}
		void setCallingConvention(CallingConvention c);

// Specify arguments
		template <typename T> void setArgument(const T &arg) {
			setArgument<T>(_args.size(),arg);
		}
		template <typename T> void setArgument(std::size_t i,const T &arg) {
			typedef typename std::decay<const T>::type TT;
			
			static_assert(sizeof(TT)==1||sizeof(TT)==2||sizeof(TT)==4||sizeof(TT)==8,
				"Unsupported argument size: 1, 2, 4 or 8 required");
			static_assert(std::is_arithmetic<TT>::value||
				std::is_pointer<TT>::value,"Argument must be of arithmetic or pointer type");
			
			auto typeSize=sizeof(TT);
			Argument::Traits t;
			if(std::is_integral<TT>::value) {
				if(std::is_signed<TT>::value) t=Argument::SignedIntegral;
				else t=Argument::UnsignedIntegral;
			}
			else if(std::is_floating_point<TT>::value) t=Argument::Float;
			else t=Argument::Pointer;
			
			auto const a=static_cast<TT>(arg);
			setArgument(i,&a,typeSize,t);
		}

// Invoke underlying function
		void invoke();

// Obtain return value
		template <typename T>
			typename std::enable_if<std::is_integral<T>::value,T>::type returnValue() const
		{
			return static_cast<T>(_retInt);
		}
		
		template <typename T>
			typename std::enable_if<std::is_same<T,float>::value,T>::type returnValue() const
		{
			return _retFloat;
		}
		
		template <typename T>
			typename std::enable_if<std::is_same<T,double>::value,T>::type returnValue() const
		{
			return _retDouble;
		}
		
		template <typename T>
			typename std::enable_if<std::is_pointer<T>::value,T>::type returnValue() const
		{
			return reinterpret_cast<T>(static_cast<MachineWord>(_retInt));
		}
	
	private:
		void allocateCodeBuffer();
		void setAccessMode(BufferAccessMode m);
		void setArgument(std::size_t i,const void *ptr,std::size_t n,Argument::Traits traits);
		void compile();
		void addByte(int ch);
		void addBytes(const void *data,std::size_t n);
		void addPointer(const void *ptr);
		void checkBuffer(std::size_t count);
	};
}

#endif
