#pragma once

/// The interface of the OS specific utility classes.
class IUtil {
public:
	virtual void Log (const string& log) = 0;
};
