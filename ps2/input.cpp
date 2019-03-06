#include "pgen.h"

// First 2 pointers are for pad 1 and 2, following 3 are for multitap pad ports
ingamePadManager *pads[5];

// Pad manager used in the GUI code
padManager *guiPad;

ingamePadManager::ingamePadManager(int port, int slot, t_keys *genKeys) : padManager(port, slot)
{
	this->genKeys = genKeys;
}

// Returns 0 for normal, 1 if select was pressed (ie: trigger ingame menu)
int ingamePadManager::updateEmulationInput()
{
	int execIngame = 0;
	int padData = updateInput();

	memset((void *)genKeys, 0, sizeof(t_keys));

	if(padData & PAD_LEFT)		genKeys->left = 1;
	if(padData & PAD_RIGHT)		genKeys->right = 1;
	if(padData & PAD_UP)		genKeys->up = 1;
	if(padData & PAD_DOWN)		genKeys->down = 1;
	if(padData & PAD_SQUARE)	genKeys->a = 1;
	if(padData & PAD_CROSS)		genKeys->b = 1;
	if(padData & PAD_CIRCLE)	genKeys->c = 1;
	if(padData & PAD_START)		genKeys->start = 1;
	if(padData & PAD_L1)		genKeys->x = 1;
	if(padData & PAD_R1)		genKeys->y = 1;
	if(padData & PAD_R2)		genKeys->z = 1;
	if(padData & PAD_L2)		genKeys->mode = 1;

	if(padData & PAD_SELECT) execIngame = 1;

	return execIngame;
}

padManager::padManager(int port, int slot)
{
	this->port = port;
	this->slot = slot;
	connected = 0;

	padBuf = (char *)memalign(64, 256);
	if(!padBuf)
		guiFatalError("Failed to allocate memory!");

	memset((void *)padBuf, 0, 256);
	int ret = padPortOpen(port, slot, padBuf);
	if(ret == 0)
		guiFatalError("Failed to open pad port!");

	dispDriver->WaitForVSync();

	if(padGetState(port, slot) == PAD_STATE_STABLE)
		padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_UNLOCK);
}

// Returns the state of the pad
int padManager::updateInput()
{
	int padData = 0;

	// Update pad status
	int padState = padGetState(port, slot);
	if((padState == PAD_STATE_STABLE) || (padState == PAD_STATE_FINDCTP1)) {
		if(connected == 0)
			padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_UNLOCK);

		connected = 1;
	} 
	else if(padState == PAD_STATE_DISCONN)
		connected = 0;

	if(connected)
	{
		struct padButtonStatus padButtons;

		if(padRead(port, slot, &padButtons) == 0)
			padData = 0;
		else
		{
			padData = 0xffff ^ padButtons.btns;

			if((padButtons.mode >> 4) == 0x07) {
				if(padButtons.ljoy_h < 64)			padData |= PAD_LEFT;
				else if(padButtons.ljoy_h > 192)	padData |= PAD_RIGHT;

				if(padButtons.ljoy_v < 64)			padData |= PAD_UP;
				else if(padButtons.ljoy_v > 192)	padData |= PAD_DOWN;
			}
		}
	}

	return padData;
}

void initPads()
{
#if 0 // XXX: multitap disabled!
	// Check for a multi-tap
	mtapPortOpen(0);
	if(mtapGetConnection(0) == 1)
		pgenRuntimeSetting.multiTapConnected = 1;
	else {
#endif
		pgenRuntimeSetting.multiTapConnected = 0;
#if 0 // XXX: multitap disabled!
		mtapPortClose(0);
	}
#endif
	
	// Create padManager objects
	pads[0] = new ingamePadManager(0, 0, &mem68k_cont.cont1[0]);	// Pad 1A
	pads[1] = new ingamePadManager(1, 0, &mem68k_cont.cont2[0]);	// Pad 2

	if(pgenRuntimeSetting.multiTapConnected)
	{
		pads[2] = new ingamePadManager(0, 1, &mem68k_cont.cont1[1]);	// Pad 1B
		pads[3] = new ingamePadManager(0, 2, &mem68k_cont.cont1[2]);	// Pad 1C
		pads[4] = new ingamePadManager(0, 3, &mem68k_cont.cont1[3]);	// Pad 1D
	}

	guiPad = pads[0];
}

void updateIngameInput()
{
	int ingame = pads[0]->updateEmulationInput();
	pads[1]->updateEmulationInput();

	if(pgenRuntimeSetting.multiTapConnected)
	{
		pads[2]->updateEmulationInput();
		pads[3]->updateEmulationInput();
		pads[4]->updateEmulationInput();
	}

	if(ingame)
		guiDoIngameMenu();
}

extern "C" int isModePressed()
{
	if(guiPad->updateInput() & PAD_SELECT)
		return 1;
	else
		return 0;
}
