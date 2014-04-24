#ifndef RecoPixelVertexing_PVClusterComparer_h
#define RecoPixelVertexing_PVClusterComparer_h
/** \class PVClusterComparer PVClusterComparer.h
 * RecoPixelVertexing/PixelVertexFinding/PVClusterComparer.h  
 * This helper class is used to sort the collection of vertexes by
 * sumPt.  It is used in DivisiveVertexFinder.  The sum of the squares
 * of the pT is only done for tracks with pT>2.5 GeV.  If the pT>10
 * GeV, then the max value of 10 is used.  (The pT of pixel tracks is
 * not very precise.)
 *
 *  \author Aaron Dominguez (UNL)
 */
#include "RecoPixelVertexing/PixelVertexFinding/interface/PVCluster.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

class PVClusterComparer {
 public:
  /// Constructor does nothing, no data members
  PVClusterComparer();
  PVClusterComparer(double track_pt_min, double track_pt_max, double track_chi2_max, double track_prob_max);
   
  /// Calculate sum of square of the pT's of the tracks in the vertex
  double pTSquaredSum(const PVCluster &v) const;
  double pTSquaredSum(const reco::Vertex &v) const;

  /// Use this operator in a std::sort to sort them in decreasing sumPt
  bool operator() (const PVCluster &v1, const PVCluster &v2) const;
  bool operator() (const reco::Vertex &v1, const reco::Vertex &v2) const;

  const double track_pT_min_;
  const double track_pT_max_;
  const double track_chi2_max_;
  const double track_prob_max_;

};
#endif
