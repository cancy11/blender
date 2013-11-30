/*
 * Copyright 2013, Blender Foundation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "particles.h"

extern "C" {
#include "DNA_object_types.h"
#include "DNA_particle_types.h"
}

namespace PTC {

using namespace Abc;
using namespace AbcGeom;

#if 0
void IParticlesSchema::init(const Abc::Argument &iArg0,
                            const Abc::Argument &iArg1)
{
	ALEMBIC_ABC_SAFE_CALL_BEGIN("IParticlesSchema::init()");

	Abc::Arguments args;
	iArg0.setInto(args);
	iArg1.setInto(args);

	AbcA::CompoundPropertyReaderPtr _this = this->getPtr();

	// no matching so we pick up old assets written as V3f
	m_positionsProperty = Abc::IP3fArrayProperty(_this, "P", kNoMatching,
	                                             args.getErrorHandlerPolicy());

	m_idsProperty = Abc::IUInt64ArrayProperty(_this, ".pointIds",
	                                          iArg0, iArg1);

	if (_this->getPropertyHeader(".velocities") != NULL) {
		m_velocitiesProperty = Abc::IV3fArrayProperty(_this, ".velocities",
		                                              iArg0, iArg1);
	}

	if (_this->getPropertyHeader(".widths" ) != NULL) {
		m_widthsParam = IFloatGeomParam(_this, ".widths", iArg0, iArg1);
	}

	ALEMBIC_ABC_SAFE_CALL_END_RESET();
}



void OParticlesSchema::set( const Sample &iSamp )
{
	ALEMBIC_ABC_SAFE_CALL_BEGIN("OParticlesSchema::set()");

	// do we need to create velocities prop?
	if (iSamp.getVelocities() && !m_velocitiesProperty) {
		m_velocitiesProperty = Abc::OV3fArrayProperty(this->getPtr(), ".velocities",
		                                              m_positionsProperty.getTimeSampling());

		std::vector<V3f> emptyVec;
		const V3fArraySample empty(emptyVec);
		const size_t numSamps = m_positionsProperty.getNumSamples();
		for (size_t i = 0 ; i < numSamps ; ++i) {
			m_velocitiesProperty.set(empty);
		}
	}

	// do we need to create widths prop?
	if (iSamp.getWidths() && !m_widthsParam) {
		std::vector<float> emptyVals;
		std::vector<Util::uint32_t> emptyIndices;
		OFloatGeomParam::Sample empty;

		if (iSamp.getWidths().getIndices()) {
			empty = OFloatGeomParam::Sample(Abc::FloatArraySample(emptyVals),
			                                Abc::UInt32ArraySample(emptyIndices),
			                                iSamp.getWidths().getScope());

			// widths are indexed which is wasteful, but technically ok
			m_widthsParam = OFloatGeomParam(this->getPtr(), ".widths", true,
			                                iSamp.getWidths().getScope(),
			                                1, this->getTimeSampling());
		}
		else {
			empty = OFloatGeomParam::Sample(Abc::FloatArraySample(emptyVals),
			                                iSamp.getWidths().getScope());

			// widths are not indexed
			m_widthsParam = OFloatGeomParam(this->getPtr(), ".widths", false,
			                                iSamp.getWidths().getScope(), 1,
			                                this->getTimeSampling());
		}

		size_t numSamples = m_positionsProperty.getNumSamples();

		// set all the missing samples
		for (size_t i = 0; i < numSamples; ++i) {
			m_widthsParam.set( empty );
		}
	}

	// We could add sample integrity checking here.
	if (m_positionsProperty.getNumSamples() == 0) {
		// First sample must be valid on all points.
		ABCA_ASSERT(iSamp.getPositions() &&
		            iSamp.getIds(),
		            "Sample 0 must have valid data for points and ids");
		m_positionsProperty.set(iSamp.getPositions());
		m_idsProperty.set(iSamp.getIds());

		if (m_velocitiesProperty) {
			m_velocitiesProperty.set(iSamp.getVelocities());
		}

		if (m_widthsParam) {
			m_widthsParam.set(iSamp.getWidths());
		}

		if (iSamp.getSelfBounds().isEmpty()) {
			// OTypedScalarProperty::set() is not referentially transparent,
			// so we need a a placeholder variable.
			Abc::Box3d bnds(ComputeBoundsFromPositions(iSamp.getPositions()));
			m_selfBoundsProperty.set( bnds );
		}
		else {
			m_selfBoundsProperty.set(iSamp.getSelfBounds());
		}
	}
	else {
		SetPropUsePrevIfNull(m_positionsProperty, iSamp.getPositions());
		SetPropUsePrevIfNull(m_idsProperty, iSamp.getIds());
		SetPropUsePrevIfNull(m_velocitiesProperty, iSamp.getVelocities());

		if (iSamp.getSelfBounds().hasVolume()) {
			m_selfBoundsProperty.set(iSamp.getSelfBounds());
		}
		else if (iSamp.getPositions()) {
			Abc::Box3d bnds(ComputeBoundsFromPositions(iSamp.getPositions()));
			m_selfBoundsProperty.set(bnds);
		}
		else {
			m_selfBoundsProperty.setFromPrevious();
		}

		if (m_widthsParam) {
			m_widthsParam.set(iSamp.getWidths());
		}
	}

	ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OParticlesSchema::setFromPrevious()
{
	ALEMBIC_ABC_SAFE_CALL_BEGIN("OParticlesSchema::setFromPrevious");

	m_positionsProperty.setFromPrevious();
	m_idsProperty.setFromPrevious();

	m_selfBoundsProperty.setFromPrevious();

	if (m_widthsParam) {
		m_widthsParam.setFromPrevious();
	}

	ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OParticlesSchema::setTimeSampling(uint32_t iIndex)
{
	ALEMBIC_ABC_SAFE_CALL_BEGIN("OParticlesSchema::setTimeSampling( uint32_t )");

	m_positionsProperty.setTimeSampling(iIndex);
	m_idsProperty.setTimeSampling(iIndex);
	m_selfBoundsProperty.setTimeSampling(iIndex);

	if (m_widthsParam)
	{
		m_widthsParam.setTimeSampling(iIndex);
	}

	ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OParticlesSchema::setTimeSampling( AbcA::TimeSamplingPtr iTime )
{
	ALEMBIC_ABC_SAFE_CALL_BEGIN("OParticlesSchema::setTimeSampling( TimeSamplingPtr )");

	if (iTime)
	{
		uint32_t tsIndex = getObject().getArchive().addTimeSampling(*iTime);
		setTimeSampling(tsIndex);
	}

	ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OParticlesSchema::init(uint32_t iTsIdx)
{
	ALEMBIC_ABC_SAFE_CALL_BEGIN("OParticlesSchema::init()");

	AbcA::MetaData mdata;
	SetGeometryScope(mdata, kVaryingScope);
	AbcA::CompoundPropertyWriterPtr _this = this->getPtr();

	m_positionsProperty = Abc::OP3fArrayProperty(_this, "P", mdata, iTsIdx);

	m_idsProperty = Abc::OUInt64ArrayProperty(_this, ".pointIds", mdata,
	                                          iTsIdx);

	ALEMBIC_ABC_SAFE_CALL_END_RESET();
}
#endif


ParticlesWriter::ParticlesWriter(const std::string &filename, Scene *scene, Object *ob, ParticleSystem *psys) :
    Writer(filename, scene),
    m_ob(ob),
    m_psys(psys)
{
	uint32_t fs = add_frame_sampling();
	
	OObject root = m_archive.getTop();
	m_points = OPoints(root, m_psys->name, fs);
}

ParticlesWriter::~ParticlesWriter()
{
}

void ParticlesWriter::write_sample()
{
	OPointsSchema &schema = m_points.getSchema();
	
	int totpart = m_psys->totpart;
	ParticleData *pa;
	int i;
	
	/* XXX TODO only needed for the first frame/sample */
	std::vector<Util::uint64_t> ids;
	ids.reserve(totpart);
	for (i = 0, pa = m_psys->particles; i < totpart; ++i, ++pa)
		ids.push_back(i);
	
	std::vector<V3f> positions;
	positions.reserve(totpart);
	for (i = 0, pa = m_psys->particles; i < totpart; ++i, ++pa) {
		float *co = pa->state.co;
		positions.push_back(V3f(co[0], co[1], co[2]));
	}
	
	OPointsSchema::Sample sample = OPointsSchema::Sample(V3fArraySample(positions), UInt64ArraySample(ids));

	schema.set(sample);
}


ParticlesReader::ParticlesReader(const std::string &filename, Scene *scene, Object *ob, ParticleSystem *psys) :
    Reader(filename, scene),
    m_ob(ob),
    m_psys(psys),
    m_totpoint(0)
{
	if (m_archive.valid()) {
		IObject root = m_archive.getTop();
		m_points = IPoints(root, m_psys->name);
	}
	
	/* XXX TODO read first sample for info on particle count and times */
	m_totpoint = 0;
}

ParticlesReader::~ParticlesReader()
{
}

PTCReadSampleResult ParticlesReader::read_sample(float frame)
{
	if (!m_points.valid())
		return PTC_READ_SAMPLE_INVALID;
	
	IPointsSchema &schema = m_points.getSchema();
	TimeSamplingPtr ts = schema.getTimeSampling();
	
	ISampleSelector ss = get_frame_sample_selector(frame);
	chrono_t time = ss.getRequestedTime();
	
	std::pair<index_t, chrono_t> sres = ts->getFloorIndex(time, schema.getNumSamples());
	chrono_t stime = sres.second;
	float sframe = time_to_frame(stime);
	
	IPointsSchema::Sample sample;
	schema.get(sample, ss);
	
	const V3f *positions = sample.getPositions()->get();
	int totpart = m_psys->totpart, i;
	ParticleData *pa;
	for (i = 0, pa = m_psys->particles; i < sample.getPositions()->size(); ++i, ++pa) {
		pa->state.co[0] = positions[i].x;
		pa->state.co[1] = positions[i].y;
		pa->state.co[2] = positions[i].z;
	}
	
	return PTC_READ_SAMPLE_EXACT;
}

} /* namespace PTC */
