/* ----------------------------------------------------------------------------
 * Copyright 2017, Massachusetts Institute of Technology,
 * Cambridge, MA 02139
 * All Rights Reserved
 * Authors: Luca Carlone, et al. (see THANKS for the full author list)
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file   OpenCvVisualizer3D.h
 * @brief  Build and visualize 3D data: 2D mesh from Frame for example.
 * @author Antoni Rosinol
 */

#pragma once

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glog/logging.h>

#include <gtsam/geometry/Point3.h>
#include <gtsam/geometry/Pose3.h>

#include <opencv2/opencv.hpp>
#include <opencv2/viz/types.hpp>

#include "kimera-vio/backend/VioBackEnd-definitions.h"
#include "kimera-vio/logging/Logger.h"
#include "kimera-vio/mesh/Mesher-definitions.h"
#include "kimera-vio/utils/Macros.h"
#include "kimera-vio/visualizer/Visualizer3D-definitions.h"
#include "kimera-vio/visualizer/Visualizer3D.h"

namespace VIO {

class OpenCvVisualizer3D : public Visualizer3D {
 public:
  KIMERA_DELETE_COPY_CONSTRUCTORS(OpenCvVisualizer3D);
  KIMERA_POINTER_TYPEDEFS(OpenCvVisualizer3D);
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  typedef size_t LineNr;
  typedef std::uint64_t PlaneId;
  typedef std::map<LandmarkId, size_t> LmkIdToLineIdMap;
  typedef std::map<PlaneId, LmkIdToLineIdMap> PlaneIdMap;
  typedef std::function<void(VisualizerOutput&)> DisplayCallback;

  /**
   * @brief Visualizer3D constructor
   * @param viz_type: type of 3D visualization
   * @param backend_type backend used so that we display the right info
   */
  OpenCvVisualizer3D(const VisualizationType& viz_type,
                     const BackendType& backend_type);
  virtual ~OpenCvVisualizer3D();

  /**
   * @brief registerMesh3dVizProperties (Legacy) this was used to paint the
   * mesh with semantic labels if sent by third-party.
   * @param cb callback
   */
  inline void registerMesh3dVizProperties(
      Mesh3dVizPropertiesSetterCallback cb) {
    mesh3d_viz_properties_callback_ = cb;
  }

  /**
   * \brief Returns true if visualization is ready, false otherwise.
   * The actual visualization must be done in the main thread, and as such,
   * it is not done here to separate visualization preparation from display.
   */
  VisualizerOutput::UniquePtr spinOnce(const VisualizerInput& input) override;

  // TODO(marcus): Is there any reason the following two methods must be
  // private?

 public:
  // Visualization calls are public in case the user wants to manually visualize
  // things, instead of running spinOnce and do it automatically.

  static Mesh3DVizProperties texturizeMesh3D(const Timestamp& image_timestamp,
                                             const cv::Mat& texture_image,
                                             const Mesh2D& mesh_2d,
                                             const Mesh3D& mesh_3d);

  /**
   * @brief addPoseToTrajectory Add pose to the previous trajectory.
   * @param current_pose_gtsam Pose to be added
   * @param img Optional img to be displayed at the pose's frustum.
   */
  void addPoseToTrajectory(const cv::Affine3d& pose);

  /**
   * @brief visualizeTrajectory3D
   * Visualize currently stored 3D trajectory (user needs to add poses with
   * addPoseToTrajectory).
   * @param frustum_image
   * @param widgets_map
   */
  void visualizeTrajectory3D(WidgetsMap* widgets_map);

  /**
   * @brief visualizeTrajectoryWithFrustums
   * Visualize trajectory with frustums, but unfortunately no image can be
   * displayed inside it.
   * @param widgets_map
   * @param n_last_frustums
   */
  void visualizeTrajectoryWithFrustums(WidgetsMap* widgets_map,
                                       const size_t& n_last_frustums = 10u);

  /**
   * @brief drawScene Draws a 3D Scene with a world frame of references, a
   * camera frame of reference and the frustum of the camera with an image
   * displayed inside the frustum
   * @param extrinsics Camera 3D extrinsics wrt World frame.
   * @param intrinsics Camera intrinsics
   * @param frustum_img Image to be displayed inside the camera frustum
   * @param widgets_map
   */
  void drawScene(const gtsam::Pose3& extrinsics,
                 const gtsam::Cal3_S2& intrinsics,
                 const cv::Mat& frustum_img,
                 WidgetsMap* widgets_map);

  /**
 * @brief visualizePoseWithImgInFrustum
 * Visualize a single camera pose with an image inside its frustum.
   * Adds an image to the frustum of the last pose if cv::Mat is not empty.
 * @param frustum_image
 * @param frustum_pose
 * @param widgets_map
 * @param widget_id Keep this id the same if you want to update the widget pose
 * and image, instead of adding a different instance.
 */
  void visualizePoseWithImgInFrustum(
      const cv::Mat& frustum_image,
      const cv::Affine3d& frustum_pose,
      WidgetsMap* widgets_map,
      const std::string& widget_id = "Camera Pose with Frustum",
      const cv::Matx33d K =
          cv::Matx33d(458, 0.0, 360, 0.0, 458, 240, 0.0, 0.0, 1.0));

  /**
   * @brief visualizePlyMesh Visualize a PLY from filename (absolute path).
   * @param filename Absolute path to ply file
   * @param widgets output
   */
  void visualizePlyMesh(const std::string& filename, WidgetsMap* widgets);

  /**
   * @brief visualizePointCloud Given a cv::Mat with each col being
   * @param[in] point_cloud A mat of type
   * cv::Mat(1, points.size(), CV_32FC3);
   * @param widgets modifies the widgets map, adds a new pointcloud.
   * @param[in] colors (optional) If using colored point cloud:
   * cv::Mat(1, points.size(), CV_8UC3);
   * @param[in] normals (optional) If using normals:
   * cv::Mat(1, points.size(), CV_32FC3);
   */
  void visualizePointCloud(const cv::Mat& point_cloud,
                           WidgetsMap* widgets,
                           const cv::Affine3d& pose = cv::Affine3d(),
                           const cv::Mat& colors = cv::Mat(),
                           const cv::Mat& normals = cv::Mat());

  void visualizeGlobalFrameOfReference(WidgetsMap* widgets, double scale = 1.0);

  /**
   * @brief visualizeMesh3D
   * Visualize a 3D point cloud of unique 3D landmarks with its connectivity,
   * and provide color for each polygon.
   * @param map_points_3d
   * @param colors
   * @param polygons_mesh
   * @param widgets
   * @param tcoords Optional texture cordinates
   * @param texture Optional texture image
   * @param id Optional string id in case you want to display multiple 3D meshes
   * in the same visualization window
   * @return false if nothing to draw
   */
  bool visualizeMesh3D(const cv::Mat& map_points_3d,
                       const cv::Mat& colors,
                       const cv::Mat& polygons,
                       WidgetsMap* widgets,
                       const cv::Mat& tcoords = cv::Mat(),
                       const cv::Mat& texture = cv::Mat(),
                       const string& id = "");

 private:
  //! Create a 2D mesh from 2D corners in an image, coded as a Frame class
  static cv::Mat visualizeMesh2D(
      const std::vector<cv::Vec6f>& triangulation2D,
      const cv::Mat& img,
      const KeypointsCV& extra_keypoints = KeypointsCV());

  //! Visualize 2d mesh.
  static cv::Mat visualizeMesh2DStereo(
      const std::vector<cv::Vec6f>& triangulation_2D,
      const Frame& ref_frame);

 private:
  //! Visualize a 3D point cloud of unique 3D landmarks.
  void visualizePoints3D(const PointsWithIdMap& points_with_id,
                         const LmkIdToLmkTypeMap& lmk_id_to_lmk_type_map,
                         WidgetsMap* widgets_map);

  //! Visualize a 3D point cloud of unique 3D landmarks with its connectivity.
  void visualizePlane(const PlaneId& plane_index,
                      const double& n_x,
                      const double& n_y,
                      const double& n_z,
                      const double& d,
                      WidgetsMap* widgets_map,
                      const bool& visualize_plane_label = true,
                      const int& cluster_id = 1);

  /**
   * @brief drawCylinder
   * @param[in] cylinder_id Unique identifier (otw it overwrites viz).
   * @param[in] axis_point1 A point1 on the axis of the cylinder.
   * @param[in] axis_point2 A point2 on the axis of the cylinder.
   * @param[in] radius Radius of the cylinder.
   * @param[out] widgets
   * @param numsides Resolution of the cylinder.
   * @param color Color of the cylinder.
   */
  void drawCylinder(const std::string& id,
                    const cv::Point3d& axis_point1,
                    const cv::Point3d& axis_point2,
                    const double& radius,
                    WidgetsMap* widgets,
                    const int& numsides = 30,
                    const cv::viz::Color& color = cv::viz::Color::red());

  //! Draw a line in opencv.
  void drawLine(const std::string& line_id,
                const double& from_x,
                const double& from_y,
                const double& from_z,
                const double& to_x,
                const double& to_y,
                const double& to_z,
                WidgetsMap* widgets);

  //! Same as above but with different interface
  void drawLine(const std::string& line_id,
                const cv::Point3d& pt1,
                const cv::Point3d& pt2,
                WidgetsMap* widgets);

  void drawArrow(const std::string& arrow_id,
                 const cv::Point3f& from,
                 const cv::Point3f& to,
                 WidgetsMap* widgets,
                 const bool& with_text = false,
                 const double& arrow_thickness = 1.0,
                 const double& text_thickness = 1.0,
                 const cv::viz::Color& color = cv::viz::Color::red());

  /**
   * @brief visualizeMesh3D Visualize a 3D point cloud of unique 3D landmarks
   * with its connectivity with only one color.
   * @param map_points_3d
   * @param polygons
   * @param widgets
   */
  void visualizeMesh3D(const cv::Mat& map_points_3d,
                       const cv::Mat& polygons,
                       WidgetsMap* widgets);

  /// Visualize a 3D point cloud of unique 3D landmarks with its connectivity.
  /// Each triangle is colored depending on the cluster it is in, or gray if it
  /// is in no cluster.
  /// [in] clusters: a set of triangle clusters. The ids of the triangles must
  ///  match the order in polygons_mesh.
  /// [in] map_points_3d: set of 3d points in the mesh, format is n rows, with
  ///  three columns (x, y, z).
  /// [in] polygons_mesh: mesh faces, format is n rows, 1 column,
  ///  with [n id_a id_b id_c, ..., n /id_x id_y id_z], where n = polygon size
  ///  n=3 for triangles.
  /// [in] color_mesh whether to color the mesh or not
  /// [in] timestamp to store the timestamp of the mesh when logging the mesh.
  void visualizeMesh3DWithColoredClusters(
      const std::vector<Plane>& planes,
      const cv::Mat& map_points_3d,
      const cv::Mat& polygons_mesh,
      WidgetsMap* widgets,
      const bool visualize_mesh_with_colored_polygon_clusters = false,
      const Timestamp& timestamp = 0.0);

  //! Visualize convex hull in 2D for set of points in triangle cluster,
  //! projected along the normal of the cluster.
  void visualizeConvexHull(const TriangleCluster& cluster,
                           const cv::Mat& map_points_3d,
                           const cv::Mat& polygons_mesh,
                           WidgetsMap* widgets);

  //!!!!!!!! Remove widget. True if successful, false if not.
  bool removeWidget(const std::string& widget_id);

  //! Visualize line widgets from plane to lmks.
  //! Point key is required to avoid duplicated lines!
  void visualizePlaneConstraints(const PlaneId& plane_id,
                                 const gtsam::Point3& normal,
                                 const double& distance,
                                 const LandmarkId& lmk_id,
                                 const gtsam::Point3& point,
                                 WidgetsMap* widgets);

  //! Remove line widgets from plane to lmks, for lines that are not pointing
  //! to any lmk_id in lmk_ids.
  void removeOldLines(const LandmarkIds& lmk_ids);

  //! Remove line widgets from plane to lmks.
  void removePlaneConstraintsViz(const PlaneId& plane_id);

  //! Remove plane widget.
  void removePlane(const PlaneId& plane_index,
                   const bool& remove_plane_label = true);

  // Render window with drawn objects/widgets.
  // @param wait_time Amount of time in milliseconds for the event loop to keep
  // running.
  // @param force_redraw If true, window renders.
  void renderWindow(int wait_time = 1, bool force_redraw = true);

  // Get a screenshot of the window.
  void getScreenshot(const std::string& filename);

  // Useful for when testing on servers without display screen.
  void setOffScreenRendering();

  // Record video sequence at a hardcoded directory relative to executable.
  void recordVideo();

 private:
  //! Flags for visualization behaviour.
  const BackendType backend_type_;

  //! Callbacks.
  //! Mesh 3d visualization properties setter callback.
  Mesh3dVizPropertiesSetterCallback mesh3d_viz_properties_callback_;

  std::deque<cv::Affine3d> trajectory_poses_3d_;

  std::map<PlaneId, LineNr> plane_to_line_nr_map_;
  PlaneIdMap plane_id_map_;
  std::map<PlaneId, bool> is_plane_id_in_window_;

  //! Colors
  cv::viz::Color cloud_color_ = cv::viz::Color::white();

  //! Logging instance.
  std::unique_ptr<VisualizerLogger> logger_;

  //! Log mesh to ply file.
  void logMesh(const cv::Mat& map_points_3d,
               const cv::Mat& colors,
               const cv::Mat& polygons_mesh,
               const Timestamp& timestamp,
               bool log_accumulated_mesh = false);

  //! Input the mesh points and triangle clusters, and
  //! output colors matrix for mesh visualizer.
  //! This will color the point with the color of the last plane having it.
  void colorMeshByClusters(const std::vector<Plane>& planes,
                           const cv::Mat& map_points_3d,
                           const cv::Mat& polygons_mesh,
                           cv::Mat* colors) const;

  //! Decide color of the cluster depending on its id.
  void getColorById(const size_t& id, cv::viz::Color* color) const;

  //! Draw a line from lmk to plane center.
  void drawLineFromPlaneToPoint(const std::string& line_id,
                                const double& plane_n_x,
                                const double& plane_n_y,
                                const double& plane_n_z,
                                const double& plane_d,
                                const double& point_x,
                                const double& point_y,
                                const double& point_z,
                                WidgetsMap* widgets);

  //! Update line from lmk to plane center.
  void updateLineFromPlaneToPoint(const std::string& line_id,
                                  const double& plane_n_x,
                                  const double& plane_n_y,
                                  const double& plane_n_z,
                                  const double& plane_d,
                                  const double& point_x,
                                  const double& point_y,
                                  const double& point_z,
                                  WidgetsMap* widgets);
};

}  // namespace VIO
