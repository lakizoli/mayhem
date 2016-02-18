#pragma once

/// The interface of the OS specific utility classes.
class IUtil {
public:
	/// Write the given string to the log.
	virtual void Log (const string& log) = 0;

	/// Returns the current time in seconds.
	virtual double GetTime () const = 0;
};
