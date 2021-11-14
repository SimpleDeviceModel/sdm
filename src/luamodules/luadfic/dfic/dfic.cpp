/*
 * Copyright (c) 2015-2021 by Microproject LLC
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
 * (foreign function interface) implementation.
 * 
 * This module provides an implementation of the Interface class.
 */

#include "dfic.h"

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
#else // not Windows
	#include <unistd.h>
	#include <stdlib.h>
	#include <sys/mman.h>
#endif

#include <utility>

using namespace Dfic;

/*
 * Public members
 */

Interface::Interface() {
	allocateCodeBuffer();
	reset();
}

Interface::Interface(const Interface &orig):
	_func(orig._func),
	_cc(orig._cc),
	_retInt(orig._retInt),
	_retFloat(orig._retFloat),
	_retDouble(orig._retDouble)
{
// _dirty is true by default
	allocateCodeBuffer();
	for(auto const &arg: orig._args)
		_args.push_back(std::unique_ptr<Argument>(new Argument(*arg)));
}

Interface::Interface(Interface &&orig):
	_args(std::move(orig._args)),
	_func(orig._func),
	_cc(orig._cc),
	_retInt(orig._retInt),
	_retFloat(orig._retFloat),
	_retDouble(orig._retDouble),
	_code(orig._code),
	_codeAccess(orig._codeAccess),
	_codeBufSize(orig._codeBufSize)
{
	orig._code=nullptr;
// _dirty is true by default
}

#ifdef _WIN32

Interface::~Interface() {
	if(_code) {
		setAccessMode(ReadWrite);
		VirtualFree(_code,0,MEM_RELEASE);
	}
}

#else // not Windows

Interface::~Interface() {
	if(_code) {
		setAccessMode(ReadWrite);
		free(_code);
	}
}

#endif

Interface &Interface::operator=(const Interface &other) {
	for(auto const &arg: other._args)
		_args.push_back(std::unique_ptr<Argument>(new Argument(*arg)));
	
	_func=other._func;
	_cc=other._cc;
	_retInt=other._retInt;
	_retFloat=other._retFloat;
	_retDouble=other._retDouble;
	_dirty=true;
	
	return *this;
}

Interface &Interface::operator=(Interface &&other) {
	_args=std::move(other._args);
	_func=other._func;
	_cc=other._cc;
	_retInt=other._retInt;
	_retFloat=other._retFloat;
	_retDouble=other._retDouble;
	_dirty=true;
	
	return *this;
}

void Interface::reset() {
	_args.clear();
	_func=nullptr;

#ifdef DFIC_CPU_X86
	_cc=CDecl;
#else
	_cc=X64Native;
#endif

	_dirty=true;
}

void Interface::setCallingConvention(CallingConvention c) {
#ifdef DFIC_CPU_X86
	if(c==X64Native) throw std::runtime_error("Unsupported calling convention");
#else
	if(c!=X64Native) throw std::runtime_error("Unsupported calling convention");
#endif
	_cc=c;
	_dirty=true;
}

void Interface::setArgument(std::size_t i,const void *ptr,std::size_t n,Argument::Traits traits) {
	if(i>_args.size()) throw std::runtime_error("Bad argument number");
	if(n==0) throw std::runtime_error("Argument size can't be zero");

// Either create new argument or check that the existing one has correct type
	if(i==_args.size()) { // Create new argument
		_args.push_back(std::unique_ptr<Argument>(new Argument(n,traits)));
		_dirty=true;
	}
	else { // check the existing one
		if(n!=_args[i]->typeSize||traits!=_args[i]->traits)
			throw std::runtime_error("Wrong argument type");
	}
	
	auto &arg=*_args[i];
	
// Initialize storage
	if(n<arg.buf.size()) {
		char init=0;
		if(arg.traits==Argument::SignedIntegral&&(static_cast<const char*>(ptr)[n-1]&0x80)) init=-1;
		std::memset(arg.buf.data(),init,arg.buf.size());
	}

// Copy data
	std::memcpy(arg.buf.data(),ptr,n);
}

void Interface::invoke() {
	if(_dirty) compile();
	setAccessMode(Execute);
	toFuncPtr(_code)();
}

/*
 * Private members
 */

#ifdef _WIN32

void Interface::allocateCodeBuffer() {
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	_codeBufSize=static_cast<std::size_t>(si.dwPageSize);
	_code=static_cast<char *>(VirtualAlloc(NULL,_codeBufSize,MEM_COMMIT|MEM_RESERVE,PAGE_NOACCESS));
	if(!_code) throw std::runtime_error("Cannot allocate memory region");
}

void Interface::setAccessMode(BufferAccessMode m) {
	if(_codeAccess!=m) {
		DWORD prot=PAGE_NOACCESS;
		DWORD oldProt;
		if(m==ReadWrite) prot=PAGE_READWRITE;
		else if(m==Execute) prot=PAGE_EXECUTE;
		BOOL b=VirtualProtect(_code,_codeBufSize,prot,&oldProt);
		if(!b) throw std::runtime_error("Cannot change memory permissions");
		_codeAccess=m;
		if(m==Execute) FlushInstructionCache(GetCurrentProcess(),_code,_codeBufSize);
	}
}

#else // not Windows

void Interface::allocateCodeBuffer() {
	_codeBufSize=static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
	void *mem;
	int r=posix_memalign(&mem,_codeBufSize,_codeBufSize);
	if(r) throw std::runtime_error("Cannot allocate aligned buffer");
	_code=static_cast<char*>(mem);
}

void Interface::setAccessMode(BufferAccessMode m) {
	if(_codeAccess!=m) {
		int prot=PROT_NONE;
		if(m==ReadWrite) prot=PROT_READ|PROT_WRITE;
		else if(m==Execute) prot=PROT_EXEC;
		int r=mprotect(_code,_codeBufSize,prot);
		if(r) throw std::runtime_error("Cannot change memory permissions");
		_codeAccess=m;
	}
}

#endif

#ifdef DFIC_CPU_X86

void Interface::compile() {
	setAccessMode(ReadWrite);
	_currentCode=_code;
	
	std::size_t stackOffset=0;
	
	std::vector<bool> viaRegs(_args.size()); // remember arguments passed via regs
	
	if(_cc==FastCall) {
		std::size_t regArgs=0;
		for(std::size_t i=0;i<_args.size();i++) {
			auto const &arg=*_args[i];
			if(arg.buf.size()<=sizeof(MachineWord)&&arg.traits!=Argument::Float) {
				if(regArgs==0) {// pass via ecx
					addBytes("\x8B\x0D",2); addPointer(arg.buf.data()); // mov ecx, arg.buf.data()
					viaRegs[i]=true;
					regArgs++;
				}
				else if(regArgs==1) {// pass via edx
					addBytes("\x8B\x15",2); addPointer(arg.buf.data()); // mov edx, arg.buf.data()
					viaRegs[i]=true;
					regArgs++;
				}
				else break;
			}
		}
	}
	else if(_cc==ThisCall) {
		if(_args.empty())
			throw std::runtime_error("thiscall convention requires at least one argument");
		
		auto const &arg=*_args[0];
		if(arg.traits!=Argument::Pointer)
			throw std::runtime_error("thiscall convention requires that the first argument is of pointer type");
		
		addBytes("\x8B\x0D",2); addPointer(arg.buf.data()); // mov ecx, arg.buf.data()
		viaRegs[0]=true;
	}
	
// Determine argument order on the stack
	int first,pastend,step;
	if(_cc!=Pascal) { // use C-style parameter passing (reverse order)
		first=static_cast<int>(_args.size())-1;
		pastend=-1;
		step=-1;
	}
	else { // use Pascal-style parameter passing (direct order)
		first=0;
		pastend=static_cast<int>(_args.size());
		step=1;
	}

// Push arguments
	for(int i=first;i!=pastend;i+=step) {
		if(viaRegs[i]) continue;
		auto const &buf=_args[i]->buf;
		for(auto it=buf.crbegin();it!=buf.crend();it++) {
			addBytes("\xFF\x35",2); addPointer(&(*it)); // push _args[i]->buf
		}
		stackOffset+=(buf.size())*sizeof(MachineWord);
	}

// Invoke function wrapper
	addBytes("\xFF\x15",2); addPointer(&_func); // call _func

// Clean up stack (if needed)
	if(_cc==CDecl) {
		addBytes("\x81\xC4",2); addBytes(&stackOffset,4); // add esp, stackOffset
	}

// Store integer return value
	addByte(0xA3); addPointer(&_retInt); // mov _retInt, eax
	addBytes("\x89\x15",2); addPointer(reinterpret_cast<char*>(&_retInt)+4); // mov ((uint32_t*)&retInt)[1], edx

// Check whether FPU stack contains return value
	addBytes("\xD9\xE5",2); // fxam
	addBytes("\xDF\xE0",2); // fnstsv ax
	addBytes("\x25\x00\x45\x00\x00",5); // and eax, 0x4500
	addBytes("\x35\x00\x41\x00\x00",5); // xor eax, 0x4100

// If FPU stack is not empty, store the FPU return value
	addBytes("\x74\x0C",2); // jz +12
	addBytes("\xD9\x15",2); addPointer(&_retFloat); // fst _retFloat
	addBytes("\xDD\x1D",2); addPointer(&_retDouble); // fstp _retDouble

// Return
	addByte(0xC3); // ret
	
	_dirty=false;
}

#else // not x86

void Interface::compile() {
	setAccessMode(ReadWrite);
	_currentCode=_code;
	
	std::size_t stackSize;
	
#ifdef _WIN32

// Register arguments
	for(std::size_t i=0;i<_args.size()&&i<4;i++) {
		auto &arg=*_args[i];
		if(arg.traits==Argument::Float) { // floating-point arguments are passed in xmm0-xmm3
			addBytes("\x48\xA1",2); addPointer(arg.buf.data()); // mov rax,arg.buf.data()
			
			char sse_modrm,gpr_rex,gpr_modrm;
			switch(i) {
			case 0: // xmm0
				sse_modrm='\xC0';
				gpr_rex='\x48';
				gpr_modrm='\xC8';
				break;
			case 1: // xmm1
				sse_modrm='\xC8';
				gpr_rex='\x48';
				gpr_modrm='\xD0';
				break;
			case 2: // xmm2
				sse_modrm='\xD0';
				gpr_rex='\x4C';
				gpr_modrm='\xC0';
				break;
			default: // xmm3
				sse_modrm='\xD8';
				gpr_rex='\x4C';
				gpr_modrm='\xC8';
				break;
			}
			addBytes("\x66\x48\x0F\x6E",4); addByte(sse_modrm); // movq <xmm>, rax
			addByte(gpr_rex); addByte(0x8B); addByte(gpr_modrm); // mov <gpr>, rax
			
		}
		else { // integral and pointer arguments are passed via general-purpose registers
			addBytes("\x48\xA1",2); addPointer(arg.buf.data()); // mov rax, arg.buf.data()
			char rex,modrm;
			switch(i) {
			case 0: // rcx
				rex='\x48';
				modrm='\xC8';
				break;
			case 1: // rdx
				rex='\x48';
				modrm='\xD0';
				break;
			case 2: // r8
				rex='\x4C';
				modrm='\xC0';
				break;
			default: // r9
				rex='\x4C';
				modrm='\xC8';
				break;
			}
			addByte(rex); addByte(0x8B); addByte(modrm); // mov <gpr>, rax
		}
	}
	
	stackSize=_args.size()*8;
	if(stackSize<32) stackSize=32;
	
	std::size_t stackPadding=0;
	if(stackSize%16!=8) { // stack must be aligned to 16-byte boundary, account for return address
		stackSize+=8;
		stackPadding=8;
	}
	
// Align stack to 16-byte boundary
	if(stackPadding) {
		addBytes("\x48\x83\xEC",3); addBytes(&stackPadding,1); // sub esp, stackPadding
	}
	
// Stack arguments
	if(_args.size()>4) {
		for(std::size_t i=_args.size()-1;i>=4;i--) {
			auto const &arg=*_args[i];
			addBytes("\x48\xA1",2); addPointer(arg.buf.data()); // mov rax, arg.buf.data()
			addByte(0x50); // push rax
		}
	}

// Shadow stack frame
	addBytes("\x48\x83\xEC\x20",4); // sub esp, 32
	
// Invoke function
	addBytes("\x48\xB8",2); addPointer(toDataPtr(_func)); // mov rax, _func
	addBytes("\xFF\xD0",2); // call rax

#else // not Windows
	std::size_t gprCount=0;
	std::size_t sseCount=0;
	std::size_t stackArgs=0;
	
	std::vector<bool> viaRegs(_args.size()); // remember arguments passed via regs
	
	for(std::size_t i=0;i<_args.size();i++) {
		auto const &arg=*_args[i];
		if(arg.traits==Argument::Float&&sseCount<8) { // pass floating point arguments via xmm
			addBytes("\x48\xA1",2); addPointer(arg.buf.data()); // mov rax,arg.buf.data()
			addBytes("\x66\x48\x0F\x6E",4); addByte(static_cast<int>(0xC0+sseCount*8)); // mov <xmm>, rax
			viaRegs[i]=true;
			sseCount++;
		}
		else if(arg.traits!=Argument::Float&&gprCount<6) {
			addBytes("\x48\xA1",2); addPointer(arg.buf.data()); // mov rax, arg.buf.data()
			char rex,modrm;
			switch(gprCount) {
			case 0: // rdi
				rex='\x48';
				modrm='\xF8';
				break;
			case 1: // rsi
				rex='\x48';
				modrm='\xF0';
				break;
			case 2: // rdx
				rex='\x48';
				modrm='\xD0';
				break;
			case 3: // rcx
				rex='\x48';
				modrm='\xC8';
				break;
			case 4: // r8
				rex='\x4C';
				modrm='\xC0';
				break;
			default: // r9
				rex='\x4C';
				modrm='\xC8';
				break;
			}
			addByte(rex); addByte(0x8B); addByte(modrm); // mov <gpr>, rax
			viaRegs[i]=true;
			gprCount++;
		}
		else { // pass via memory
			stackArgs++;
		}
	}
	
	stackSize=stackArgs*8;
	std::size_t stackPadding=0;
	if(stackSize%16!=8) {
		stackSize+=8;
		stackPadding=8;
	}
	
// Align stack to 16-byte boundary
	if(stackPadding) {
		addBytes("\x48\x83\xEC",3); addBytes(&stackPadding,1); // sub esp, stackPadding
	}

// Stack arguments
	if(stackArgs) {
		for(int i=static_cast<int>(_args.size())-1;i>=0;i--) {
			auto const &arg=*_args[i];
			if(viaRegs[i]) continue;
			addBytes("\x48\xA1",2); addPointer(arg.buf.data()); // mov rax, arg.buf.data()
			addByte(0x50); // push rax
		}
	}
	
// Store number of XMM registers used to pass parameters (needed for vararg functions)
	addByte(0xB0); addBytes(&sseCount,1); // mov al, sseCount%256

// Invoke function. Note: r11 is used here since it is a caller-save register
// which is not used for parameter passing
	addBytes("\x49\xBB",2); addPointer(toDataPtr(_func)); // mov r11, _func
	addBytes("\x49\xFF\xD3",3); // call r11

#endif
	
// Clean up stack
	addBytes("\x48\x81\xC4",3); addBytes(&stackSize,4); // add esp, stackSize

// Store return value
	addBytes("\x48\xA3",2); addPointer(&_retInt); // mov _retInt, rax
	addBytes("\x48\xB8",2); addPointer(&_retFloat); // mov rax, &_retFloat
	addBytes("\x66\x0F\x7E\x00",4); // movd [rax], xmm0
	addBytes("\x48\xB8",2); addPointer(&_retDouble); // mov rax, &_retDouble
	addBytes("\x66\x48\x0F\x7E\x00",5); // movq [rax], xmm0
	
// Return
	addByte(0xC3); // ret

	_dirty=false;
}

#endif

inline void Interface::addByte(int ch) {
	checkBuffer(1);
	*_currentCode=static_cast<char>(ch);
	_currentCode++;
}

inline void Interface::addBytes(const void *data,std::size_t n) {
	checkBuffer(n);
	std::memcpy(_currentCode,data,n);
	_currentCode+=n;
}

inline void Interface::addPointer(const void *ptr) {
	checkBuffer(sizeof(const void*));
	std::memcpy(_currentCode,&ptr,sizeof(const void *));
	_currentCode+=sizeof(const void *);
}

inline void Interface::checkBuffer(std::size_t count) {
	auto fill=static_cast<char*>(_currentCode)-static_cast<char*>(_code);
	if(static_cast<std::size_t>(fill)+count>_codeBufSize)
		throw std::runtime_error("Insufficient code buffer");
}
