//##########################################################################
//#                                                                        #
//#                       CLOUDCOMPARE PLUGIN: qPCL                        #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 or later of the License.      #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                         COPYRIGHT: Luca Penasa                         #
//#                                                                        #
//##########################################################################
//
#include "sm2cc.h"

//Local
#include "PCLConv.h"
#include "my_point_types.h"

//PCL
#include <pcl/common/io.h>

//qCC_db
#include <ccPointCloud.h>
#include <ccScalarField.h>

//system
#include <list>

//PCL V1.6 or older
#ifdef PCL_VER_1_6_OR_OLDER

#include <sensor_msgs/PointField.h>
typedef sensor_msgs::PointField PCLScalarField;

#else //Version 1.7 or newer

#include <pcl/PCLPointField.h>
typedef pcl::PCLPointField PCLScalarField;

#endif

//system
#include <assert.h>

// Custom PCL point type with integer coordinates
struct _PointXYZInteger
{
  union EIGEN_ALIGN16
  {
    std::int32_t data[3];
    struct
	{
      std::int32_t x;
      std::int32_t y;
      std::int32_t z;
    };
  };

  PCL_MAKE_ALIGNED_OPERATOR_NEW
};

struct EIGEN_ALIGN16 PointXYZInteger : public _PointXYZInteger
{
	PointXYZInteger()
	{
		x = y = z = 0;
	}
};

POINT_CLOUD_REGISTER_POINT_STRUCT(	_PointXYZInteger,
									(std::int32_t, x, x)
									(std::int32_t, y, y)
									(std::int32_t, z, z) )

POINT_CLOUD_REGISTER_POINT_WRAPPER(PointXYZInteger, _PointXYZInteger)

size_t GetNumberOfPoints(const PCLCloud& pclCloud)
{
	return static_cast<size_t>(pclCloud.width) * pclCloud.height;
}

bool ExistField(const PCLCloud& pclCloud, std::string name)
{
	for (const auto& field : pclCloud.fields)
		if (field.name == name)
			return true;

	return false;
}

bool pcl2cc::CopyXYZ(const PCLCloud& pclCloud, ccPointCloud& ccCloud, bool hasIntegerCoordinates)
{
	size_t pointCount = GetNumberOfPoints(pclCloud);
	if (pointCount == 0)
	{
		assert(false);
		return false;
	}

	if (!ccCloud.reserve(static_cast<unsigned>(pointCount)))
	{
		return false;
	}

	//add xyz to the input cloud taking xyz infos from the sm cloud
	if (hasIntegerCoordinates)
	{
		pcl::PointCloud<PointXYZInteger> pcl_cloud;
		FROM_PCL_CLOUD(pclCloud, pcl_cloud);
		for (size_t i = 0; i < pointCount; ++i)
		{
			CCVector3 P(pcl_cloud.at(i).x,
						pcl_cloud.at(i).y,
						pcl_cloud.at(i).z);

			ccCloud.addPoint(P);
		}
	}
	else
	{
		pcl::PointCloud<pcl::PointXYZ>::Ptr pcl_cloud(new pcl::PointCloud<pcl::PointXYZ>);
		FROM_PCL_CLOUD(pclCloud, *pcl_cloud);
		//loop
		for (size_t i = 0; i < pointCount; ++i)
		{
			CCVector3 P(pcl_cloud->at(i).x,
						pcl_cloud->at(i).y,
						pcl_cloud->at(i).z);

			ccCloud.addPoint(P);
		}
	}

	return true;
}

bool pcl2cc::CopyNormals(const PCLCloud& pclCloud, ccPointCloud& ccCloud)
{
	pcl::PointCloud<OnlyNormals>::Ptr pcl_cloud_normals (new pcl::PointCloud<OnlyNormals>);
	FROM_PCL_CLOUD(pclCloud, *pcl_cloud_normals);

	if (!ccCloud.reserveTheNormsTable())
		return false;

	size_t pointCount = GetNumberOfPoints(pclCloud);

	//loop
	for (size_t i = 0; i < pointCount; ++i)
	{
		CCVector3 N(	static_cast<PointCoordinateType>(pcl_cloud_normals->at(i).normal_x),
						static_cast<PointCoordinateType>(pcl_cloud_normals->at(i).normal_y),
						static_cast<PointCoordinateType>(pcl_cloud_normals->at(i).normal_z) );

		ccCloud.addNorm(N);
	}

	ccCloud.showNormals(true);
	
	return true;
}

bool pcl2cc::CopyRGB(const PCLCloud& pclCloud, ccPointCloud& ccCloud)
{
	pcl::PointCloud<OnlyRGB>::Ptr pcl_cloud_rgb (new pcl::PointCloud<OnlyRGB>);
	FROM_PCL_CLOUD(pclCloud, *pcl_cloud_rgb);
	size_t pointCount = GetNumberOfPoints(pclCloud);
	if (pointCount == 0)
		return true;
	if (!ccCloud.reserveTheRGBTable())
		return false;


	//loop
	for (size_t i = 0; i < pointCount; ++i)
	{
		ccColor::Rgb C(	static_cast<ColorCompType>(pcl_cloud_rgb->points[i].r),
						static_cast<ColorCompType>(pcl_cloud_rgb->points[i].g),
						static_cast<ColorCompType>(pcl_cloud_rgb->points[i].b) );
		ccCloud.addColor(C);
	}

	ccCloud.showColors(true);

	return true;
}

bool pcl2cc::CopyScalarField(	const PCLCloud& pclCloud,
								const std::string& sfName,
								ccPointCloud& ccCloud,
								bool overwriteIfExist/*=true*/)
{
	//if the input field already exists...
	int id = ccCloud.getScalarFieldIndexByName(sfName.c_str());
	if (id >= 0)
	{
		if (overwriteIfExist)
			//we simply delete it
			ccCloud.deleteScalarField(id);
		else
			//we keep it as is
			return false;
	}

	size_t pointCount = GetNumberOfPoints(pclCloud);

	//create new scalar field
	ccScalarField* cc_scalar_field = new ccScalarField(sfName.c_str());
	if (!cc_scalar_field->reserveSafe(static_cast<unsigned>(pointCount)))
	{
		cc_scalar_field->release();
		return false;
	}

	//get PCL field
	int field_index = pcl::getFieldIndex(pclCloud, sfName);
	const PCLScalarField& pclField = pclCloud.fields[field_index];
	//temporary change the name of the given field to something else -> S5c4laR should be a pretty uncommon name,
	const_cast<PCLScalarField&>(pclField).name = "S5c4laR";

	switch (pclField.datatype)
	{
	case PCLScalarField::FLOAT32:
	{
		pcl::PointCloud<FloatScalar>::Ptr pcl_scalar(new pcl::PointCloud<FloatScalar>);
		FROM_PCL_CLOUD(pclCloud, *pcl_scalar);

		for (size_t i = 0; i < pointCount; ++i)
		{
			ScalarType scalar = static_cast<ScalarType>(pcl_scalar->points[i].S5c4laR);
			cc_scalar_field->addElement(scalar);
		}
	}
	break;

	case PCLScalarField::FLOAT64:
	{
		pcl::PointCloud<DoubleScalar>::Ptr pcl_scalar(new pcl::PointCloud<DoubleScalar>);
		FROM_PCL_CLOUD(pclCloud, *pcl_scalar);

		for (size_t i = 0; i < pointCount; ++i)
		{
			ScalarType scalar = static_cast<ScalarType>(pcl_scalar->points[i].S5c4laR);
			cc_scalar_field->addElement(scalar);
		}
	}
	break;

	case PCLScalarField::INT16:
	{
		pcl::PointCloud<ShortScalar>::Ptr pcl_scalar(new pcl::PointCloud<ShortScalar>);
		FROM_PCL_CLOUD(pclCloud, *pcl_scalar);

		for (size_t i = 0; i < pointCount; ++i)
		{
			ScalarType scalar = static_cast<ScalarType>(pcl_scalar->points[i].S5c4laR);
			cc_scalar_field->addElement(scalar);
		}
	}
	break;

	case PCLScalarField::UINT16:
	{
		pcl::PointCloud<UShortScalar>::Ptr pcl_scalar(new pcl::PointCloud<UShortScalar>);
		FROM_PCL_CLOUD(pclCloud, *pcl_scalar);

		for (size_t i = 0; i < pointCount; ++i)
		{
			ScalarType scalar = static_cast<ScalarType>(pcl_scalar->points[i].S5c4laR);
			cc_scalar_field->addElement(scalar);
		}
	}
	break;

	case PCLScalarField::UINT32:
	{
		pcl::PointCloud<UIntScalar>::Ptr pcl_scalar(new pcl::PointCloud<UIntScalar>);
		FROM_PCL_CLOUD(pclCloud, *pcl_scalar);

		for (size_t i = 0; i < pointCount; ++i)
		{
			ScalarType scalar = static_cast<ScalarType>(pcl_scalar->points[i].S5c4laR);
			cc_scalar_field->addElement(scalar);
		}
	}
	break;

	case PCLScalarField::INT32:
	{
		pcl::PointCloud<IntScalar>::Ptr pcl_scalar(new pcl::PointCloud<IntScalar>);
		FROM_PCL_CLOUD(pclCloud, *pcl_scalar);

		for (size_t i = 0; i < pointCount; ++i)
		{
			ScalarType scalar = static_cast<ScalarType>(pcl_scalar->points[i].S5c4laR);
			cc_scalar_field->addElement(scalar);
		}
	}
	break;

	default:
		ccLog::Warning(QString("[PCL] Field with an unmanaged type (= %1)").arg(pclField.datatype));
		cc_scalar_field->release();
		return false;
	}

	cc_scalar_field->computeMinAndMax();
	ccCloud.addScalarField(cc_scalar_field);
	ccCloud.setCurrentDisplayedScalarField(0);
	ccCloud.showSF(true);

	//restore old name for the scalar field
	const_cast<PCLScalarField&>(pclField).name = sfName;

	return true;
}

ccPointCloud* pcl2cc::Convert(const PCLCloud& pclCloud)
{
	//retrieve the valid fields
	std::list<std::string> fields;
	bool hasIntegerCoordinates = false;
	for (const auto& field : pclCloud.fields)
	{
		if (field.name != "_") //PCL padding fields
		{
			fields.push_back(field.name);
		}

		if (!hasIntegerCoordinates
			&&	field.name == "x"
			&&	field.datatype < pcl::PCLPointField::FLOAT32)
		{
			hasIntegerCoordinates = true;
		}
	}

	//begin with checks and conversions
	//be sure we have x, y, and z fields
	if (!ExistField(pclCloud, "x") || !ExistField(pclCloud, "y") || !ExistField(pclCloud, "z"))
	{
		return nullptr;
	}

	//create cloud
	ccPointCloud* ccCloud = new ccPointCloud();
	size_t expectedPointCount = GetNumberOfPoints(pclCloud);
	if (expectedPointCount != 0)
	{
		//push points inside
		if (!CopyXYZ(pclCloud, *ccCloud, hasIntegerCoordinates))
		{
			delete ccCloud;
			return nullptr;
		}
	}

	//remove x,y,z fields from the vector of field names
	fields.remove("x");
	fields.remove("y");
	fields.remove("z");

	//do we have normals?
	if (ExistField(pclCloud, "normal_x") || ExistField(pclCloud, "normal_y") || ExistField(pclCloud, "normal_z"))
	{
		CopyNormals(pclCloud, *ccCloud);

		//remove the corresponding fields
		fields.remove("normal_x");
		fields.remove("normal_y");
		fields.remove("normal_z");
	}

	//The same for colors
	if (ExistField(pclCloud, "rgb"))
	{
		CopyRGB(pclCloud, *ccCloud);

		//remove the corresponding field
		fields.remove("rgb");
	}
	//The same for colors
	else if (ExistField(pclCloud, "rgba"))
	{
		CopyRGB(pclCloud, *ccCloud);

		//remove the corresponding field
		fields.remove("rgba");
	}

	//All the remaining fields will be stored as scalar fields
	for (const std::string& name : fields)
	{
		CopyScalarField(pclCloud, name, *ccCloud);
	}

	return ccCloud;
}
