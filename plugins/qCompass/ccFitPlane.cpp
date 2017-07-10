#include "ccFitPlane.h"
#include "ccCompass.h"
ccFitPlane::ccFitPlane(ccPlane* p)
	: ccPlane(*p) //copy the passed ccPlane object
{
	//add metadata tag defining the ccCompass class type
	QVariantMap* map = new QVariantMap();
	map->insert("ccCompassType", "FitPlane");
	setMetaData(*map, true);

	//update name
	CCVector3 N(getNormal());
	//We always consider the normal with a positive 'Z' by default!
	if (N.z < 0.0)
		N *= -1.0;
	//calculate strike/dip/dip direction
	float strike, dip, dipdir;
	ccNormalVectors::ConvertNormalToDipAndDipDir(N, dip, dipdir);
	ccNormalVectors::ConvertNormalToStrikeAndDip(N, strike, dip);
	QString dipAndDipDirStr = QString("%1/%2").arg((int)dip, 2, 10, QChar('0')).arg((int)dipdir, 3, 10, QChar('0'));

	setName(dipAndDipDirStr);

	//update drawing properties based on ccCompass state
	enableStippling(ccCompass::drawStippled);
	showNameIn3D(ccCompass::drawName);
	showNormalVector(ccCompass::drawNormals);
}

ccFitPlane::~ccFitPlane()
{
}

void ccFitPlane::updateAttributes(float rms, float search_r)
{
	//calculate and store plane attributes
	//get plane normal vector
	CCVector3 N(getNormal());
	//We always consider the normal with a positive 'Z' by default!
	if (N.z < 0.0)
		N *= -1.0;

	//calculate strike/dip/dip direction
	float strike, dip, dipdir;
	ccNormalVectors::ConvertNormalToDipAndDipDir(N, dip, dipdir);
	ccNormalVectors::ConvertNormalToStrikeAndDip(N, strike, dip);

	//calculate centroid
	CCVector3 C = getCenter();

	//store attributes (centroid, strike, dip, RMS) on plane
	QVariantMap* map = new QVariantMap();
	map->insert("Cx", C.x); map->insert("Cy", C.y); map->insert("Cz", C.z); //centroid
	map->insert("Nx", N.x); map->insert("Ny", N.y); map->insert("Nz", N.z); //normal
	map->insert("Strike", strike); map->insert("Dip", dip); map->insert("DipDir", dipdir); //strike & dip
	map->insert("RMS", rms); //rms
	map->insert("Radius", search_r); //search radius
	setMetaData(*map, true);
}

bool ccFitPlane::isFitPlane(ccHObject* object)
{
	if (object->hasMetaData("ccCompassType"))
	{
		return object->getMetaData("ccCompassType").toString().contains("FitPlane");
	}
	return false;
	/*return object->isKindOf(CC_TYPES::PLANE) //ensure object is a plane
	&& object->hasMetaData("Cx") //ensure plane has the correct metadata
	&& object->hasMetaData("Cy")
	&& object->hasMetaData("Cz")
	&& object->hasMetaData("Nx")
	&& object->hasMetaData("Ny")
	&& object->hasMetaData("Nz")
	&& object->hasMetaData("Strike")
	&& object->hasMetaData("Dip")
	&& object->hasMetaData("DipDir")
	&& object->hasMetaData("RMS")
	&& object->hasMetaData("Radius");*/
}

ccFitPlane* ccFitPlane::Fit(CCLib::GenericIndexedCloudPersist* cloud, double *rms)
{
	ccPlane* p = ccPlane::Fit(cloud, rms);
	if (p) //valid plane
	{
		ccFitPlane* fp = new ccFitPlane(p);
		p->transferChildren(*fp);
		return fp;
	}
	else
	{
		return nullptr; //return null
	}
}