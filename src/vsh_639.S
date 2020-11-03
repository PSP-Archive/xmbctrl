	.set noreorder
	
#include "pspstub.s"

	STUB_START "vsh",0x00090011,0x00020005
	STUB_FUNC  0x3A370539,vshGetRegistryValue
	STUB_FUNC  0xCD3AF2EC,vshSetRegistryValue
	STUB_END
