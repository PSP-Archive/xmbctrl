	.set noreorder

#include "pspstub.s"

STUB_START "VshCtrlLib",0x40090000,0x00020005 
STUB_FUNC 0xFD26DA72, vctrlVSHRegisterVshMenu
STUB_FUNC 0xCD6B3913, vctrlVSHExitVSHMenu
STUB_END 
