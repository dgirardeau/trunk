//##########################################################################
//#                                                                        #
//#                              CLOUDCOMPARE                              #
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
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

//Always first
#include "StBuilding.h"

StBuilding::StBuilding(const QString& name)
	: ccHObject(name)
{
}

StBuilding::StBuilding(StBuilding& obj)
	: ccHObject(obj)
{
	
}

StBuilding::StBuilding(ccHObject& obj)
	: ccHObject(obj)
{
}

StBuilding::~StBuilding()
{
}