#pragma once

#include "../management/IUtil.h"

class AndroidUtil : public IUtil {
public:
	virtual void Log (const string& log) override;
};
