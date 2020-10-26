#pragma once
#include "Src.h"

class ShaderChief final{
	cstr shaderFilePaths[3];
	uint shaderProgID;
	static uint currID;
	void Init();
	void ParseShader(cstr, uint) const;
	void LinkProg() const;
public:
	ShaderChief(cstr, cstr, cstr = "");
	~ShaderChief();
	void UseProg();

	///Utility functions
	static void SetUni1f(cstr, float, bool = 1);
	static void SetUniMtx4fv(cstr, float*, bool = 1);
	static void SetUni2f(cstr, float, float, bool = 1);
	static void SetUni3f(cstr, float, float, float, bool = 1);
	static void SetUni4f(cstr, float[4], bool = 1);
	static void SetUni1i(cstr, int, bool = 1);
};