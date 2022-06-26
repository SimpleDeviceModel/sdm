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
 * DllDepTool is a tool for Windows deployment automation.
 * It scans the executable modules (EXE/DLL) and copies all the
 * DLLs required to run them.
 */

#include "peimports.h"
#include "winpath.h"
#include "u8efile.h"
#include "u8eio.h"
#include "u8ecodec.h"
#include "u8eenv.h"

#include <iostream>
#include <cstring>
#include <cstdlib>

using namespace u8e;

struct ModuleInfo {
	std::string strName;
	std::string strSourceDir;
	std::string strDestDir;
	PEType type;
	bool skip;
	
	std::string srcPath() const {return strSourceDir+"\\"+strName;}
	std::string dstPath() const {return strDestDir+"\\"+strName;}
};

bool noVCRuntime=false;
auto const msdlls={"msvcp","msvcr","vccorlib","vcamp","vcomp",
	"mfc","mfcm","msdia","atl","vcruntime","concrt","api-ms-win-crt-"};

std::vector<ModuleInfo> processedModules;
std::vector<ModuleInfo> deployedModules;

ModuleValidation validateModule(const std::string &str) {
	std::string strFileName=strtolower(str);
	if(strFileName.rfind(".dll")!=strFileName.size()-4) return nodecision;
	
	if(strFileName=="msvcrt.dll") return forbid;

// Compare file names against known Microsoft Visual C++ redistributable DLL names
	for(auto const &msdll: msdlls) {
		if(strFileName.find(msdll)==0) { // Microsoft runtime DLL detected
			if(noVCRuntime) return forbid;
			if(strFileName.find("d.dll")!=std::string::npos) {
				utf8cout()<<"WARNING: debug versions of Microsoft Visual C++ Runtime "
				"are not redistributable ("<<strFileName<<")"<<endl;
				return forbid;
			}
			else return force;
		}
	}
	
	return nodecision;
}

int processModule(const std::string &strFileName,const std::string &destDir,const std::vector<std::string> &libpath,PEType arch=unknown,bool skip=false) {
	ModuleInfo current;
	
	splitPath(strFileName,current.strSourceDir,current.strName);
	current.strDestDir=destDir;
	current.skip=skip;
	
// analyze PE image
	PEImage img;
	int r=img.load(strFileName);
	if(r) {
		utf8cout()<<"ERROR: Can't process ["<<strFileName<<"]"<<endl;
		return -1;
	}
	
	auto dlls=img.getDlls();
	current.type=img.getArch();
	
	if(arch!=unknown&&arch!=current.type) return -1; // wrong type

// Already processed this source?
	std::vector<ModuleInfo>::const_iterator it;
	for(it=processedModules.begin();it!=processedModules.end();it++) {
		if(it->strName==current.strName&&
			it->strSourceDir==current.strSourceDir&&
			it->strDestDir==current.strDestDir) break;
	}
	if(it==processedModules.end()) processedModules.push_back(current);
	else return 0;
	
	utf8cout()<<"Processing ["<<current.strSourceDir<<"\\"<<current.strName<<"] ..."<<endl;
	
	auto path=getEnvPath();
	auto syspath=getSysPath(current.type);
	
	IFileStream test;
	std::string testpath;
	
	for(auto it=dlls.cbegin();it!=dlls.cend();it++) {
		ModuleValidation v=validateModule(*it);
		if(v==forbid) continue;
	// Check the destination directory
		testpath=destDir+"\\"+(*it);
		test.open(testpath.c_str(),std::ios_base::in);
		if(test) {
			test.close();
			int r=processModule(testpath,destDir,libpath,current.type);
			if(!r) continue;
		}
		test.clear();
	// Check additional library paths (if any)
		std::vector<std::string>::const_iterator pit;
		for(pit=libpath.begin();pit!=libpath.end();pit++) {
			testpath=(*pit)+"\\"+(*it);
			test.open(testpath.c_str(),std::ios_base::in);
			if(test) {
				test.close();
				int r=processModule(testpath,destDir,libpath,current.type);
				if(!r) break;
				else break;
			}
			test.clear();
		}
		if(pit!=libpath.end()) continue;
	// Check windows and windows/system directories
		for(pit=syspath.begin();pit!=syspath.end();pit++) {
			testpath=(*pit)+"\\"+(*it);
			test.open(testpath.c_str(),std::ios_base::in);
			if(test) {
				test.close();
				if(v==force) {
					int r=processModule(testpath,destDir,libpath,current.type);
					if(!r) break;
				}
				else break;
			}
			test.clear();
		}
		if(pit!=syspath.end()) continue;
	// Check PATH
		for(pit=path.begin();pit!=path.end();pit++) {
			testpath=(*pit)+"\\"+(*it);
			test.open(testpath.c_str(),std::ios_base::in);
			if(test) {
				test.close();
				int r=processModule(testpath,destDir,libpath,current.type);
				if(!r) break;
			}
			test.clear();
		}
		if(pit==path.end()) utf8cout()<<"WARNING: could not find DLL \""<<(*it)<<"\""<<endl;
	}
	
	return 0;
}

int deploymentPlan() {
	IFileStream test;
	std::string testpath;
	
	for(auto it=processedModules.cbegin();it!=processedModules.cend();it++) {
		if(it->skip) continue;
		if(it->strSourceDir==it->strDestDir) continue;
		testpath=it->dstPath();
		test.open(testpath.c_str(),std::ios_base::in);
		if(test) {
			utf8cout()<<"["<<testpath<<"] already here, skipping"<<endl;
			test.close();
			continue;
		}
// Check if already deployed
		std::vector<ModuleInfo>::const_iterator dit;
		for(dit=deployedModules.begin();dit!=deployedModules.end();dit++) {
			if(it->dstPath()==dit->dstPath()) {
				if(it->type!=dit->type) {
					utf8cout()<<"FATAL ERROR: modules with different architectures "
					"\""<<it->srcPath()<<"\" ("<<pestr(it->type)<<") and \""<<dit->srcPath()<<
					"\" ("<<pestr(dit->type)<<") can't be deployed "
					"at the same location \""<<it->dstPath()<<"\""<<endl;
					return -1;
				}
				else if(it->srcPath()!=dit->srcPath()) {
					utf8cout()<<"WARNING: \""<<it->srcPath()<<"\" won't be deployed at \""<<
					it->dstPath()<<"\" since \""<<dit->srcPath()<<"\" is already deployed there"<<endl;
					break;
				}
			}
		}
		if(dit==deployedModules.end()) deployedModules.push_back(*it);
	}
	
	return 0;
}

void displayusage(const std::string &strProgramName) {
	utf8cout()<<"Usage: "<<strProgramName<<" [options] <file1> [file2 [file3 ... ] ]"<<endl<<endl;
	utf8cout()<<"Options:"<<endl;
	utf8cout()<<"\t--dest-dir <path>    Destination directory to store dependencies"<<endl;
	utf8cout()<<"\t                     (by default the same directory as analyzed file)"<<endl;
	utf8cout()<<"\t--libpath <path>     Additional paths to search for dependencies"<<endl;
	utf8cout()<<"\t                     (can be used multiple times)"<<endl;
	utf8cout()<<"\t--no-vc-runtime      Ignore Visual Studio runtime DLLs"<<endl;
	utf8cout()<<endl;
	utf8cout()<<"Wildcards are supported."<<endl;
}

int main(int argc,char *argv[]) {
	utf8cout()<<"DllDepTool: collects DLLs required for the software to run"<<endl;
	utf8cout()<<"Copyright (c) 2015 Simple Device Model contributors"<<endl;
	utf8cout()<<"Note: please make sure that you are legally allowed to redistribute the DLLs!"<<endl<<endl;
	
	auto args=cmdArgs(argc,argv);
	
	if(args.size()<2) {
		displayusage(args[0]);
		return 0;
	}
	
	std::size_t firstFileName=1;
	std::string strDestDir;
	std::vector<std::string> strLibraryPaths;
	
	for(std::size_t i=1;i<args.size();i++) {
		if(args[i]=="--dest-dir") {
			i++;
			if(args.size()<i+2) {
				displayusage(args[0]);
				return EXIT_FAILURE;
			}
			
			strDestDir=normalizePath(args[i]);
		}
		else if(args[i]=="--libpath") {
			i++;
			if(args.size()<i+2) {
				displayusage(args[0]);
				return EXIT_FAILURE;
			}
			
			strLibraryPaths.push_back(normalizePath(args[i]));
		}
		else if(args[i]=="--no-vc-runtime") noVCRuntime=true;
		else {
			firstFileName=i;
			break;
		}
	}
	
	utf8cout()<<"Analyzing dependencies..."<<endl;
	
	for(std::size_t i=firstFileName;i<args.size();i++) {
		auto expanded=processWildcards(args[i]);
		for(auto it=expanded.cbegin();it!=expanded.cend();it++) {
			std::string strDir,strFileName;
			splitPath(it->c_str(),strDir,strFileName);
			if(processModule(*it,strDestDir.empty()?strDir:strDestDir,strLibraryPaths,unknown,true)) {
				utf8cout()<<"Aborting..."<<endl;
				return EXIT_FAILURE;
			}
		}
	}
	
	if(processedModules.empty()) {
		utf8cout()<<"Nothing was processed"<<endl;
		return 0;
	}
	
	utf8cout()<<"Preparing deployment plan..."<<endl;
	
	int r=deploymentPlan();
	
	if(r) return EXIT_FAILURE;
	
	if(deployedModules.empty()) {
		utf8cout()<<"Nothing to deploy"<<endl;
		return 0;
	}
	
	utf8cout()<<"Copying files..."<<endl;
	
	WCodec codec(UTF8);
	
	for(auto it=deployedModules.cbegin();it!=deployedModules.cend();it++) {
		std::wstring src=codec.transcode(it->srcPath());
		codec.reset();
		std::wstring dst=codec.transcode(it->dstPath());
		codec.reset();
		BOOL b=CopyFileW(src.c_str(),dst.c_str(),TRUE);
		if(b) utf8cout()<<"Copied ["<<it->srcPath()<<"] to ["<<it->dstPath()<<"]"<<endl;
		else utf8cout()<<"ERROR: can't copy ["<<it->srcPath()<<"] to ["<<it->dstPath()<<"]"<<endl;
	}
	
	return 0;
}
