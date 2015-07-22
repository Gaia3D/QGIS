
SET (NGI_SRCS qgsngiprovider.cpp QgsNGIDataSource.cpp QgsNGILayer.cpp QgsNGILayerData.cpp QgsNGIFeatureIterator.cpp)

SET(NGI_MOC_HDRS QgsNGIProvider.h)

########################################################
# Build

QT4_WRAP_CPP(NGI_MOC_SRCS ${NGI_MOC_HDRS})

INCLUDE_DIRECTORIES(
  .
  ../../core
  ${CMAKE_CURRENT_SOURCE_DIR}/../../core/symbology-ng
)


ADD_LIBRARY(ngiprovider MODULE ${NGI_SRCS} ${NGI_MOC_SRCS})

TARGET_LINK_LIBRARIES(ngiprovider
  qgis_core
)
IF (MSVC)
  #needed for linking to gdal which needs odbc
  SET(TARGET_LINK_LIBRARIES ${TARGET_LINK_LIBRARIE} odbc32 odbccp32)
ENDIF (MSVC)

INSTALL (TARGETS ngiprovider
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

