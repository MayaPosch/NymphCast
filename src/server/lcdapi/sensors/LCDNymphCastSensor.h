/* 
	LCDNymphCastSensor.h - Header file for NymphCast sensor for LCDApi. 
	
	Revision 0
	
	Features:
			- For use with the NymphCast server.
			- API for obtaining the currently playing media's information (if any).
			
	2021/11/21, Maya Posch
*/

#ifndef _LCDAPI_SENSORS_LCDNYMPHCASTSENSOR_H_
#define _LCDAPI_SENSORS_LCDNYMPHCASTSENSOR_H_

#include <lcdapi/sensors/LCDSensor.h>
#include <string>

namespace lcdapi {

/** \class LCDNymphCastSensor LCDNymphCastSensor.h "api/LCDNymphCastSensor.h"
 *  \brief A sensor for NymphCast multimedia media information.
 *  \ingroup sensors
 *  This sensor tries to get the title of the current
 *  song.
 *  A default string can be used if nothing appropriate is found.
 */
class LCDNymphCastSensor : public LCDSensor {
 private:
  std::string _previousValue;
  std::string _defaultValue;
 public:
  virtual void waitForChange();
  virtual std::string getCurrentValue();
  /**
   * \brief Default constructor.
   *
   * Used to build such a sensor.
   * @param defaultValue A string containing the value that will be used if
   * there is no active playback.
   */
  explicit LCDNymphCastSensor(const std::string& defaultValue = "");
  virtual ~LCDNymphCastSensor();
};

} // end of lcdapi namespace

#endif
