#pragma once
#ifndef ES_CORE_UTILS_TIME_UTIL_H
#define ES_CORE_UTILS_TIME_UTIL_H

#include <string>

namespace Utils
{
	namespace Time
	{
		static int NOT_A_DATE_TIME = 0;

		class DateTime
		{
		public:

			 DateTime();
			 DateTime(const time_t& _time);
			 DateTime(const tm& _timeStruct);
			 DateTime(const std::string& _isoString);
			~DateTime();

			const bool operator<           (const DateTime& _other) const { return (mTime <  _other.mTime); }
			const bool operator<=          (const DateTime& _other) const { return (mTime <= _other.mTime); }
			const bool operator>           (const DateTime& _other) const { return (mTime >  _other.mTime); }
			const bool operator>=          (const DateTime& _other) const { return (mTime >= _other.mTime); }
			           operator time_t     ()                       const { return mTime; }
			           operator tm         ()                       const { return mTimeStruct; }
			           operator std::string()                       const { return mIsoString; }

			void               setTime      (const time_t& _time);
			const time_t&      getTime      () const { return mTime; }
			void               setTimeStruct(const tm& _timeStruct);
			const tm&          getTimeStruct() const { return mTimeStruct; }
			void               setIsoString (const std::string& _isoString);
			const std::string& getIsoString () const { return mIsoString; }

		private:

			time_t      mTime;
			tm          mTimeStruct;
			std::string mIsoString;

		}; // DateTime

		class Duration
		{
		public:

			 Duration(const time_t& _time);
			~Duration();

			unsigned int getDays   () const { return mDays; }
			unsigned int getHours  () const { return mHours; }
			unsigned int getMinutes() const { return mMinutes; }
			unsigned int getSeconds() const { return mSeconds; }

		private:

			unsigned int mTotalSeconds;
			unsigned int mDays;
			unsigned int mHours;
			unsigned int mMinutes;
			unsigned int mSeconds;

		}; // Duration

		time_t      now         ();
		time_t      stringToTime(const std::string& _string, const std::string& _format = "%Y%m%dT%H%M%S");
		std::string timeToString(const time_t& _time, const std::string& _format = "%Y%m%dT%H%M%S");
		int         daysInMonth (const int _year, const int _month);
		int         daysInYear  (const int _year);

	} // Time::

} // Utils::

#endif // ES_CORE_UTILS_TIME_UTIL_H
