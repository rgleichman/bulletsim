#include "correction.h"
#include <iostream>
#include "vector_io.h"
#include "utils_pcl.h"
#include "my_assert.h"
#include <Eigen/Dense>
using namespace Eigen;
using namespace std;
using namespace pcl;

MatrixXf toEigenMatrix(const vector< vector<float> >& in) {
  ASSERT(in.size() > 1) ;
  MatrixXf out(in.size(),in[0].size()); 
  for (int i=0; i<in.size(); i++) 
    for (int j=0; j<in[0].size(); j++)
      out(i,j) = in[i][j];
  return out;
}

int main() {


  {
    MatrixXf coefs = toEigenMatrix(floatMatFromFile("/home/joschu/cpp/clouds/kinect_correction_identity.txt"));
    MatrixXf A1(6,3);
    A1.setRandom();
    MatrixXf A2 = correctPoints(A1,coefs);
    ASSERT(A1==A2);
  }

  {
    MatrixXf coefs = toEigenMatrix(floatMatFromFile("/home/joschu/cpp/clouds/kinect_correction_coefs.txt"));
    MatrixXf A1(6,3);
    A1.setRandom();
    MatrixXf A2 = correctPoints(A1,coefs);
    ASSERT(A1!=A2);
  }

  MatrixXf coefs = toEigenMatrix(floatMatFromFile("/home/joschu/cpp/clouds/kinect_correction_identity.txt"));
  ColorCloudPtr cloud1 = readPCD("/home/joschu/cpp/clouds/test.pcd");
  ColorCloudPtr cloud2 = correctCloudXYZRGB(cloud1,coefs);
  for (int i=0; i < cloud1->size(); i++) {
    PointXYZRGB& pt1 = cloud1->at(i);
    PointXYZRGB& pt2 = cloud2->at(i);
    ASSERT(pt1.x == pt2.x);
    ASSERT(pt1.y == pt2.y);
    ASSERT(pt1.z == pt2.z);
    ASSERT(pt1.r == pt2.r);
    ASSERT(pt1.g == pt2.g);
    ASSERT(pt1.b == pt2.b);
  }
}