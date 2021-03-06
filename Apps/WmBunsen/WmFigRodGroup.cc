#include "WmFigRodGroup.hh"

WmFigRodGroup::WmFigRodGroup() : m_isReadingFromCache( false ), m_simulationNeedsReset( false )
{
}

WmFigRodGroup::~WmFigRodGroup()
{
    removeAllRods();
}

int WmFigRodGroup::addRod()
{
    // This is a place holder rod really has no data but exists so that the input curves/strand
    // indices line up with which rod is which.
    RodData* rodData = new RodData();
    m_rodData.push_back( rodData );

    return (int)m_rodData.size() - 1;
}

int WmFigRodGroup::addRod( std::vector<BASim::Vec3d>& i_rodVertices, RodOptions& i_rodOptions, 
                           double i_massDamping, BASim::Vec3d& i_gravity,  
                           RodTimeStepper::Method i_solverType, const bool i_isFromCache,
                           const bool i_doReverseHairdo )
{
    // We have rod vertices so lets build a real rod.
    
    RodData* rodData = new RodData( i_rodOptions, i_rodVertices, i_massDamping, i_gravity, 
                                    i_solverType, i_isFromCache, i_doReverseHairdo );

    m_rodData.push_back( rodData );

    return (int)m_rodData.size() - 1;
}
