// This file is part of openCaesar3.
//
// openCaesar3 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// openCaesar3 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with openCaesar3.  If not, see <http://www.gnu.org/licenses/>.

#include "advisor_education_window.hpp"
#include "gfx/decorator.hpp"
#include "core/gettext.hpp"
#include "pushbutton.hpp"
#include "label.hpp"
#include "game/resourcegroup.hpp"
#include "core/stringhelper.hpp"
#include "gfx/engine.hpp"
#include "game/enums.hpp"
#include "game/city.hpp"
#include "building/house.hpp"
#include "core/foreach.hpp"
#include "game/settings.hpp"

namespace gui
{

namespace {
  struct InfrastructureInfo
  {
    int buildingCount;
    int buildingWork;
    int peoplesStuding;
    int coverage;
  };
}

class EducationInfoLabel : public Label
{
public:
  EducationInfoLabel( Widget* parent, const Rect& rect, const TileOverlayType service,
                      const InfrastructureInfo& info )
    : Label( parent, rect )
  {
    _service = service;
    _info = info;

    setFont( Font::create( FONT_1_WHITE ) );
  }

  virtual void _updateTexture( GfxEngine& painter )
  {
    Label::_updateTexture( painter );

    std::string buildingStr, peoplesStr;
    switch( _service )
    {
    case B_SCHOOL: buildingStr = _("##schools##"); peoplesStr = _("##children##"); break;
    case B_COLLEGE: buildingStr = _("##colleges##"); peoplesStr = _("##students##"); break;
    case B_LIBRARY: buildingStr = _("##libraries##"); peoplesStr = _("##peoples##"); break;
    default: break;
    }

    PictureRef& texture = getTextPicture();
    Font font = getFont();
    std::string buildingStrT = StringHelper::format( 0xff, "%d %s", _info.buildingCount, buildingStr.c_str() );
    font.draw( *texture, buildingStrT, 0, 0 );

    std::string buildingWorkT = StringHelper::format( 0xff, "%d", _info.buildingWork );
    font.draw( *texture, buildingWorkT, 165, 0 );

    std::string peoplesStrT = StringHelper::format( 0xff, "%d %s", _info.peoplesStuding, peoplesStr.c_str() );
    font.draw( *texture, peoplesStrT, 255, 0 );

    const char* coverages[10] = { "##edu_poor##", "##edu_very_bad##", "##edu_bad##", "##edu_not_bad##", "##edu_simple##",
                                  "##edu_above_simple##", "##edu_good##", "##edu_very_good##", "##edu_pretty##", "##edu_awesome##" };
    const char* coverageStr = _info.coverage > 0
                                  ? coverages[ math::clamp( _info.coverage / 10, 0, 10 ) ]
                                  : "##non_cvrg##";
    font.draw( *texture, _( coverageStr ), 470, 0 );
  }

private:
  TileOverlayType _service;
  InfrastructureInfo _info;
};

class AdvisorEducationWindow::Impl
{
public:
  Label* cityInfo;

  EducationInfoLabel* lbSchoolInfo;
  EducationInfoLabel* lbCollegeInfo;
  EducationInfoLabel* lbLibraryInfo;

  InfrastructureInfo getInfo( CityPtr city, const TileOverlayType service );
};


AdvisorEducationWindow::AdvisorEducationWindow( CityPtr city, Widget* parent, int id ) 
: Widget( parent, id, Rect( 0, 0, 1, 1 ) ), _d( new Impl )
{
  setGeometry( Rect( Point( (parent->getWidth() - 640 )/2, parent->getHeight() / 2 - 242 ),
               Size( 640, 256 ) ) );

  setupUI( GameSettings::rcpath( "/gui/educationadv.gui" ) );

  Point startPoint( 42, 103 );
  Size labelSize( 550, 20 );
  InfrastructureInfo info = _d->getInfo( city, B_SCHOOL );
  _d->lbSchoolInfo = new EducationInfoLabel( this, Rect( startPoint, labelSize ), B_SCHOOL, info );

  info = _d->getInfo( city, B_COLLEGE );
  _d->lbCollegeInfo = new EducationInfoLabel( this, Rect( startPoint + Point( 0, 20), labelSize), B_COLLEGE, info );

  info = _d->getInfo( city, B_LIBRARY );
  _d->lbLibraryInfo = new EducationInfoLabel( this, Rect( startPoint + Point( 0, 40), labelSize), B_LIBRARY, info );

  CityHelper helper( city );

  int sumScholars = 0;
  int sumStudents = 0;
  HouseList houses = helper.find<House>( B_HOUSE );
  foreach( HousePtr house, houses )
  {
    sumScholars += house->getHabitants().count( CitizenGroup::scholar );
    sumStudents += house->getHabitants().count( CitizenGroup::student );
  }

  std::string cityInfoStr = StringHelper::format( 0xff, "%d %s, %d %s, %d %s", city->getPopulation(), _("##peoples##"),
                                                  sumScholars, _("##scholars##"), sumStudents, _("##students##") );
  _d->cityInfo = new Label( this, Rect( 65, 50, getWidth() - 65, 50 +30), cityInfoStr, false );
}

void AdvisorEducationWindow::draw( GfxEngine& painter )
{
  if( !isVisible() )
    return;

  Widget::draw( painter );
}


InfrastructureInfo AdvisorEducationWindow::Impl::getInfo(CityPtr city, const TileOverlayType service)
{
  CityHelper helper( city );

  InfrastructureInfo ret;

  ret.buildingWork = 0;
  ret.peoplesStuding = 0;
  ret.buildingCount = 0;
  ret.coverage = 0;

  ServiceBuildingList servBuildings = helper.find<ServiceBuilding>( service );
  foreach( ServiceBuildingPtr serv, servBuildings )
  {
    if( serv->getWorkers() > 0 )
    {
      ret.buildingWork++;

      int maxStuding = 0;
      switch( service )
      {
      case B_SCHOOL: maxStuding = 75; break;
      case B_COLLEGE: maxStuding = 100; break;
      case B_LIBRARY: maxStuding = 800; break;
      default: break;
      }

      ret.peoplesStuding += maxStuding * serv->getWorkers() / serv->getMaxWorkers();
    }
    ret.buildingCount++;
  }

  CitizenGroup::Age age;
  switch( service )
  {
  case B_SCHOOL: age = CitizenGroup::scholar; break;
  case B_COLLEGE: age = CitizenGroup::student; break;
  case B_LIBRARY: age = CitizenGroup::mature; break;
  default: break;
  }

  HouseList houses = helper.find<House>( B_HOUSE );
  int peoplesCount=0;
  foreach( HousePtr house, houses )
  {
    peoplesCount += house->getHabitants().count( age );
  }
  ret.coverage = ret.peoplesStuding * 100 / (peoplesCount+1);

  return ret;
}

} //end namespace gui