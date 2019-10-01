#include "UserCode/CPEanalyzer/interface/SistripOverlapHit.h"

SiStripOverlapHit::SiStripOverlapHit(TrajectoryMeasurement const& measA, TrajectoryMeasurement const& measB) {
  // check which hit is closer to the IP
  // assign it to hitA_, the other to hitB_
  double rA = measA.recHit()->globalPosition().perp();
  double rB = measB.recHit()->globalPosition().perp();
  if(rA<rB) {
    measA_ = measA;
    measB_ = measB;
  } else {
    measA_ = measB;
    measB_ = measA;
  }
}

TrajectoryStateOnSurface const& SiStripOverlapHit::trajectoryStateOnSurface(unsigned int hit, bool updated) const {
  assert(hit<2);
  switch(hit) {
    case 0:
      return updated ? measA_.updatedState() : measA_.predictedState();
    case 1:
      return updated ? measB_.updatedState() : measB_.predictedState();
    default:
      return measA_.updatedState();
  }
}

double SiStripOverlapHit::getTrackLocalAngle(unsigned int hit) const {
  //TODO check if it is barePhi or bareTheta... we need the angle along the precise coordinate (that goes to 0 for high Pt).
  return hit? trajectoryStateOnSurface(hit-1).localDirection().barePhi() : (trajectoryStateOnSurface(0).localDirection().barePhi()+trajectoryStateOnSurface(1).localDirection().barePhi())/2.;
}

double SiStripOverlapHit::offset(unsigned int hit) const {
  assert(hit<2);
  //TODO check if it is x or y... we need the precise direction and it can be checked with the error...
  return this->hit(hit)->localPosition().x()-trajectoryStateOnSurface(hit,false).localPosition().x();
}

double SiStripOverlapHit::shift() const {
  // so this is the double difference 
  return offset(0)-offset(1);
}

double SiStripOverlapHit::distance() const {
  return (hitA()->globalPosition().basicVector()  - hitB()->globalPosition().basicVector()).mag();
}

GlobalPoint SiStripOverlapHit::position() const{
  auto vector = (hitA()->globalPosition().basicVector()+hitB()->globalPosition().basicVector())/2.;
  return GlobalPoint(vector);
}
