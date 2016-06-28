/*++

� 2015 netfabb GmbH (Original Author)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of Microsoft Corporation nor netfabb GmbH nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MICROSOFT AND/OR NETFABB BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Abstract:

Converter.cpp : Can convert 3MFs to STL and back

--*/

#ifndef __GCC
#include <tchar.h>
#include <Windows.h>
#endif // __GCC

#include <time.h>
#include <iostream>
#include <string>
#include <algorithm>

// DLL Includes of 3MF Library
#include "NMR_DLLInterfaces.h"

// Use NMR namespace for the interfaces
using namespace NMR;

#ifdef __GCC

	UINT64 GetTickCount64 () {
		UINT64 nTime = time(NULL);
		return nTime * 1000;
	}

	const char * PathFindExtension (_In_ const char * pszwPath)
	{
		const char * pResult = NULL;
		const char * pChar = pszwPath;
		while (*pChar) {
			if (*pChar == '.')
				pResult = pChar;
			pChar++;
		}

		if (!pResult)
			pResult = pChar;
		return pResult;
	}

#endif

#ifndef __GCC
int _tmain(int argc, _TCHAR* argv[])
#else
int main (int argc, const char * argv[])
#endif // __GCC
{
	// General Variables
	HRESULT hResult;
	UINT64 nStartTicks;
	DWORD nInterfaceVersion;
	DWORD nErrorMessage;
	LPCSTR pszErrorMessage;

	// Objects
	PLib3MFModel * pModel;
	PLib3MFModelReader * pReader;
	PLib3MFModelWriter * pWriter;


	std::cout << "------------------------------------------------------------------" << std::endl;
	std::cout << "3MF Model Converter" << std::endl;
	std::cout << "------------------------------------------------------------------" << std::endl;


	// Check 3MF Library Version
	hResult = lib3mf_getinterfaceversion(&nInterfaceVersion);
	if (hResult != LIB3MF_OK) {
		std::cout << "could not get 3MF Library version: " << std::hex << hResult << std::endl;
		return -1;
	}

	if ((nInterfaceVersion != NMR_APIVERSION_INTERFACE)) {
		std::cout << "invalid 3MF Library version: " << nInterfaceVersion << std::endl;
		std::cout << "3MF Library version should be: " << NMR_APIVERSION_INTERFACE << std::endl;
		return -1;
	}


	// Parse Arguments
	if (argc != 2) {
		std::cout << "Usage: " << std::endl;
		std::cout << "Convert 3MF to STL: Converter.exe model.3mf" << std::endl;
		std::cout << "Convert STL to 3MF: Converter.exe model.stl" << std::endl;
		return -1;
	}

	// Extract Extension of filename
	std::string sReaderName;
	std::string sWriterName;
	std::string sNewExtension;

	std::string sFilename(argv[1]);
	std::string sExtension = PathFindExtension(sFilename.c_str());
	std::transform(sExtension.begin(), sExtension.end(), sExtension.begin(), ::tolower);

	// Which Reader and Writer classes do we need?
	if (sExtension == ".stl") {
		sReaderName = "stl";
		sWriterName = "3mf";
		sNewExtension = ".3mf";
	}
	if (sExtension == ".3mf") {
		sReaderName = "3mf";
		sWriterName = "stl";
		sNewExtension = ".stl";
	}
	if (sReaderName.length() == 0) {
		std::cout << "unknown input file extension:" << sExtension << std::endl;
		return -1;
	}

	// Create new filename
	std::string sOutputFilename = sFilename;
	sOutputFilename.erase(sOutputFilename.length() - sExtension.length());
	sOutputFilename += sNewExtension;

	// Create Model Instance
	hResult = lib3mf_createmodel(&pModel, true);
	if (hResult != LIB3MF_OK) {
		std::cout << "could not create model: " << std::hex << hResult << std::endl;
		return -1;
	}

	// Create Model Reader
	hResult = lib3mf_model_queryreader(pModel, sReaderName.c_str(), &pReader);
	if (hResult != LIB3MF_OK) {
		std::cout << "could not create model reader: " << std::hex << hResult << std::endl;
        lib3mf_getlasterror(pModel, &nErrorMessage, &pszErrorMessage);
		std::cout << "error #" << std::hex << nErrorMessage << ": " << pszErrorMessage << std::endl;
		lib3mf_release(pModel);
		return -1;
	}


	// Import Model from File
	std::cout << "reading " << sFilename << "..." << std::endl;
    nStartTicks = GetTickCount64();

	hResult = lib3mf_reader_readfromfileutf8(pReader, sFilename.c_str());
	if (hResult != LIB3MF_OK) {
		std::cout << "could not parse file: " << std::hex << hResult << std::endl;
        lib3mf_getlasterror(pReader, &nErrorMessage, &pszErrorMessage);
		std::cout << "error #" << std::hex << nErrorMessage << ": " << pszErrorMessage << std::endl;
		lib3mf_release(pReader);
		lib3mf_release(pModel);
		return -1;
	}
	std::cout << "elapsed time: " << (GetTickCount64() - nStartTicks) << "ms" << std::endl;

	// Release Model Reader
	lib3mf_release(pReader);

	// Create Model Writer
	hResult = lib3mf_model_querywriter(pModel, sWriterName.c_str(), &pWriter);
	if (hResult != LIB3MF_OK) {
		std::cout << "could not create model writer: " << std::hex << hResult << std::endl;
        lib3mf_getlasterror(pModel, &nErrorMessage, &pszErrorMessage);
		std::cout << "error #" << std::hex << nErrorMessage << ": " << pszErrorMessage << std::endl;
		lib3mf_release(pModel);
		return -1;
	}

	// Export Model into File
	std::cout << "writing " << sOutputFilename << "..." << std::endl;
	nStartTicks = GetTickCount64();
	hResult = lib3mf_writer_writetofileutf8(pWriter, sOutputFilename.c_str());
	if (hResult != LIB3MF_OK) {
		std::cout << "could not write file: " << std::hex << hResult << std::endl;
        lib3mf_getlasterror(pWriter, &nErrorMessage, &pszErrorMessage);
		std::cout << "error #" << std::hex << nErrorMessage << ": " << pszErrorMessage << std::endl;

        lib3mf_release(pWriter);
		lib3mf_release(pModel);
		return -1;
	}
	std::cout << "elapsed time: " << (GetTickCount64() - nStartTicks) << "ms" << std::endl;
	std::cout << "done" << std::endl;

	// Release Model
	lib3mf_release(pModel);

	return 0;
}

