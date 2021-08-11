package main
import (
	"github.com/MetaFFI/plugin-sdk/compiler/go"
	"github.com/MetaFFI/plugin-sdk/compiler/go/IDL"
)

import "C"

var pluginMain *LanguagePluginMain

//--------------------------------------------------------------------
type LanguagePluginMain struct{
}
//--------------------------------------------------------------------
func NewPython3LanguagePluginMain() *LanguagePluginMain{
	this := &LanguagePluginMain{}
	compiler.CreateLanguagePluginInterfaceHandler(this)
	return this
}
//--------------------------------------------------------------------
func (this *LanguagePluginMain) CompileToGuest(idlDefinition *IDL.IDLDefinition, outputPath string) error{

	cmp := NewCompiler(idlDefinition, outputPath)
	_, err := cmp.CompileGuest()
	return err
}
//--------------------------------------------------------------------
func (this *LanguagePluginMain) CompileFromHost(idlDefinition *IDL.IDLDefinition, outputPath string, hostOptions map[string]string) error{

	cmp := NewCompiler(idlDefinition, outputPath)
	_, err := cmp.CompileHost(hostOptions)
	return err
}
//--------------------------------------------------------------------
//export init_plugin
func init_plugin(){
	pluginMain = NewPython3LanguagePluginMain()
}
//--------------------------------------------------------------------
func main(){}
//--------------------------------------------------------------------
