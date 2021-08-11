package main

import (
	"github.com/MetaFFI/plugin-sdk/compiler/go/IDL"
)

//--------------------------------------------------------------------
type Compiler struct{
	def *IDL.IDLDefinition
	outputPath string
}
//--------------------------------------------------------------------
func NewCompiler(def *IDL.IDLDefinition, outputPath string) *Compiler {
	return &Compiler{def: def, outputPath: outputPath}
}
//--------------------------------------------------------------------
func (this *Compiler) CompileGuest() (outputFileName string, err error){

	cmp := NewGuestCompiler(this.def, this.outputPath, this.def.IDLFilename)
	outputFileName, err = cmp.Compile()
	if err != nil{
		return
	}

	return outputFileName, err
}
//--------------------------------------------------------------------
func (this *Compiler) CompileHost(hostOptions map[string]string) (outputFileName string, err error){

	cmp := NewHostCompiler(this.def, this.outputPath, this.def.IDLFilename, hostOptions)
	return cmp.Compile()
}
//--------------------------------------------------------------------