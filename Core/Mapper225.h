#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class Mapper225 : public BaseMapper
{
protected:
	virtual uint16_t GetPRGPageSize() { return 0x4000; }
	virtual uint16_t GetCHRPageSize() { return 0x2000; }

	void InitMapper()
	{
		SelectPRGPage(0, 0);
		SelectPRGPage(1, 1);
		SelectCHRPage(0, 0);
	}

	virtual void Reset(bool softReset)
	{
		if(softReset) {
			SelectPRGPage(0, 0);
			SelectPRGPage(1, 1);
			SelectCHRPage(0, 0);
		}
	}

	void WriteRegister(uint16_t addr, uint8_t value)
	{
		uint8_t highBit = (addr >> 8) & 0x40;
		uint8_t prgPage = ((addr >> 6) & 0x3F) | highBit;
		if(addr & 0x0100) {
			SelectPRGPage(0, prgPage);
			SelectPRGPage(1, prgPage);
		} else {
			SelectPRGPage(0, prgPage & 0xFE);
			SelectPRGPage(1, (prgPage & 0xFE) + 1);
		}

		SelectCHRPage(0, addr & 0x3F | highBit);

		SetMirroringType(addr & 0x2000 ? MirroringType::Horizontal : MirroringType::Vertical);
	}
};