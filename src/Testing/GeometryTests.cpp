// GeometryTests.cpp
#include <gtest/gtest.h>

#include "Geometry.h"
#define _USE_MATH_DEFINES

using namespace std;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
class GeometryTest : public ::testing::Test {
  protected:
    GeometryPtr test_geom_, default_geom_;
    Radius r_zero_, r_null_, r_five_, r_four_;
    Length len_zero_, len_null_, len_five_, len_four_;
    point_t origin_, x_offset_, y_offset_, z_offset_;

    virtual void SetUp(){
      r_zero_ = 0;
      r_null_ = NULL; 
      r_five_ = 5;
      r_four_ = 4;
      len_zero_ = 0;
      len_null_ = NULL; 
      len_five_ = 5;
      len_four_ = 4;
      point_t origin_ = {0,0,0};
      point_t x_offset_ = {len_five_, 0, 0};
      point_t y_offset_ = {0, len_five_, 0};
      point_t z_offset_ = {0, 0, len_five_};
      default_geom_ = GeometryPtr(new Geometry());
      test_geom_ = GeometryPtr(new Geometry(r_four_, r_five_, x_offset_, len_five_));
    }
    virtual void TearDown() {
    }
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
TEST_F(GeometryTest, defaultConstructor) {
  EXPECT_FLOAT_EQ(0, default_geom_->inner_radius());
  EXPECT_EQ( 0, default_geom_->outer_radius());

  EXPECT_FLOAT_EQ(0, default_geom_->centroid().x_);
  EXPECT_FLOAT_EQ(0, default_geom_->centroid().y_);
  EXPECT_FLOAT_EQ(0, default_geom_->centroid().z_);

  EXPECT_FLOAT_EQ(0, default_geom_->x());
  EXPECT_FLOAT_EQ(0, default_geom_->y());
  EXPECT_FLOAT_EQ(0, default_geom_->z());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
TEST_F(GeometryTest, fullConstructor) {
  EXPECT_FLOAT_EQ(r_four_,   test_geom_->inner_radius());
  EXPECT_FLOAT_EQ(r_five_,   test_geom_->outer_radius());
  EXPECT_FLOAT_EQ(len_five_, test_geom_->length());

  EXPECT_FLOAT_EQ(len_five_, test_geom_->centroid().x_);
  EXPECT_FLOAT_EQ(0,         test_geom_->centroid().y_);
  EXPECT_FLOAT_EQ(0,         test_geom_->centroid().z_);

  EXPECT_FLOAT_EQ(len_five_, test_geom_->x());
  EXPECT_FLOAT_EQ(0,         test_geom_->y());
  EXPECT_FLOAT_EQ(0,         test_geom_->z());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
TEST_F(GeometryTest, radial_midpoint ) {  
  EXPECT_FLOAT_EQ( 0, default_geom_->radial_midpoint());
  Radius expected = r_four_ + (r_five_ - r_four_)/2.0;
  EXPECT_FLOAT_EQ( expected, test_geom_->radial_midpoint() ); 
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
TEST_F(GeometryTest, solid_volume ){
  for(int r=0; r < 10; ++r) {
    double radius = r*0.5;
    for( int l=0; l < 10; ++l) { 
      double length = l*0.5;
      EXPECT_FLOAT_EQ(M_PI*radius*radius*length, test_geom_->solid_volume(radius, length));
    }
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
TEST_F(GeometryTest, volume){
  EXPECT_FLOAT_EQ(M_PI*len_five_*(r_five_*r_five_-r_four_*r_four_) , test_geom_->volume());

  EXPECT_FLOAT_EQ(0 , default_geom_->volume());
  EXPECT_NO_THROW(default_geom_->set_radius(INNER, r_four_));
  EXPECT_NO_THROW(default_geom_->set_radius(OUTER, r_five_));
  EXPECT_NO_THROW(default_geom_->set_length(len_five_));
  EXPECT_FLOAT_EQ(M_PI*len_five_*(r_five_*r_five_-r_four_*r_four_) , default_geom_->volume());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
TEST_F(GeometryTest, surface_area){
  EXPECT_FLOAT_EQ(2*M_PI*r_five_*(r_five_+len_five_) , test_geom_->surface_area());

  EXPECT_FLOAT_EQ(0 , default_geom_->surface_area());
  EXPECT_NO_THROW(default_geom_->set_radius(INNER, r_four_));
  EXPECT_NO_THROW(default_geom_->set_radius(OUTER, r_five_));
  EXPECT_NO_THROW(default_geom_->set_length(len_five_));
  EXPECT_FLOAT_EQ(2*M_PI*r_five_*(r_five_ + len_five_) , default_geom_->surface_area());
}
