#include "tracking.h"
#include "perception/config_perception.h"
#include "simulation/config_bullet.h"
#include "simulation/recording.h"
#include <BulletSoftBody/btSoftBodyHelpers.h>

int main(int argc, char* argv[]) {
  Eigen::internal::setNbThreads(3);

  Parser parser;
  GeneralConfig::scale = 10;
  parser.addGroup(TrackingConfig());
  parser.addGroup(RecordingConfig());
  parser.addGroup(SceneConfig());
  parser.addGroup(GeneralConfig());

  parser.addGroup(BulletConfig());
  parser.read(argc, argv);


  initComm();
  TowelTracker2 tv;
  extern bool LIVE;
  if (LIVE) tv.runOnline();
  else tv.runOffline();

  // extern bool LIVE;
  // if (LIVE) TrackerSystem.runOnline();
  // else TrackerSystem.runOffline();


}
