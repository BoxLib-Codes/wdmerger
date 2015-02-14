#include "Castro.H"
#include "Castro_F.H"
#include "Problem_F.H"

//
// This function computes the center-of-mass locations (and velocities of the center of masses)
// of the primary and secondary white dwarfs.
// First, we predict where the center of mass locations will be
// for these stars based on Keplerian circular orbits.
// Then, we do a location-weighted sum that considers, for each respective star,
// only the zones that are closer to one orbit location than the other.
//

void
Castro::wdCOM (Real time, Real& mass_p, Real& mass_s, Real* com_p, Real* com_s, Real* vel_p, Real* vel_s)
{
    BL_PROFILE("Castro::wdCOM()");

    const Real* dx       = geom.CellSize();

    MultiFab*   mfrho    = derive("density",time,0);
    MultiFab*   mfxmom   = derive("xmom",time,0);
    MultiFab*   mfymom   = derive("ymom",time,0);
    MultiFab*   mfzmom   = derive("zmom",time,0);

    BL_ASSERT(mfrho  != 0);
    BL_ASSERT(mfxmom != 0);
    BL_ASSERT(mfymom != 0);
    BL_ASSERT(mfzmom != 0);

    if (level < parent->finestLevel())
    {
	const MultiFab* mask = getLevel(level+1).build_fine_mask();

	MultiFab::Multiply(*mfrho,  *mask, 0, 0, 1, 0);
	MultiFab::Multiply(*mfxmom, *mask, 0, 0, 1, 0);
	MultiFab::Multiply(*mfymom, *mask, 0, 0, 1, 0);
	MultiFab::Multiply(*mfzmom, *mask, 0, 0, 1, 0);
    }

#ifdef _OPENMP
#pragma omp parallel reduction(+:com_p[0],com_p[1],com_p[2],com_s[0],com_s[1],com_s[2]) \
                     reduction(+:vel_p[0],vel_p[1],vel_p[2],vel_s[0],vel_s[1],vel_s[2]) \
                     reduction(+:mass_p, mass_s)
#endif    
    for (MFIter mfi(*mfrho,true); mfi.isValid(); ++mfi)
    {
        FArrayBox& fabrho  = (*mfrho )[mfi];
	FArrayBox& fabxmom = (*mfxmom)[mfi];
	FArrayBox& fabymom = (*mfymom)[mfi];
	FArrayBox& fabzmom = (*mfzmom)[mfi];
    
        const Box& box  = mfi.tilebox();
        const int* lo   = box.loVect();
        const int* hi   = box.hiVect();

	BL_FORT_PROC_CALL(WDCOM,wdcom)
            (BL_TO_FORTRAN(fabrho),
	     BL_TO_FORTRAN(fabxmom),
	     BL_TO_FORTRAN(fabymom),
	     BL_TO_FORTRAN(fabzmom),
	     lo,hi,dx,&time,
	     com_p, com_s,
	     vel_p, vel_s,
	     &mass_p, &mass_s);
    }

    delete mfrho;
    delete mfxmom;
    delete mfymom;
    delete mfzmom;

    ParallelDescriptor::ReduceRealSum(mass_p);
    ParallelDescriptor::ReduceRealSum(mass_s);

    ParallelDescriptor::ReduceRealSum(com_p,3);
    ParallelDescriptor::ReduceRealSum(com_s,3);
    ParallelDescriptor::ReduceRealSum(vel_p,3);
    ParallelDescriptor::ReduceRealSum(vel_s,3);

}



// This function uses the known center of mass of the two white dwarfs,
// and given a density cutoff, computes the total volume of all zones
// whose density is greater or equal to that density cutoff.
// We also impose a distance requirement so that we only look 
// at zones that are within twice the original radius of the white dwarf.

void Castro::volInBoundary (Real               time,
                	    Real*              com_p,
			    Real*              com_s,
			    Real&              vol_p,
			    Real&              vol_s,
                            Real               rho_cutoff)
{
    BL_PROFILE("Castro::volInBoundary()");

    const Real* dx      = geom.CellSize();
    MultiFab*   mf      = derive("density",time,0);

    BL_ASSERT(mf != 0);

    if (level < parent->finestLevel())
    {
	const MultiFab* mask = getLevel(level+1).build_fine_mask();
	MultiFab::Multiply(*mf, *mask, 0, 0, 1, 0);
    }

#ifdef _OPENMP
#pragma omp parallel reduction(+:sum)
#endif    
    for (MFIter mfi(*mf,true); mfi.isValid(); ++mfi)
    {
        FArrayBox& fab = (*mf)[mfi];

	Real sp = 0.0;
	Real ss = 0.0;
        const Box& box  = mfi.tilebox();
        const int* lo   = box.loVect();
        const int* hi   = box.hiVect();

	BL_FORT_PROC_CALL(CA_VOLUMEINDENSITYBOUNDARY,ca_volumeindensityboundary)
	                  (BL_TO_FORTRAN(fab),lo,hi,dx,com_p,com_s,&sp,&ss,&rho_cutoff);
        vol_p += sp;
	vol_s += ss;
    }

    delete mf;

    ParallelDescriptor::ReduceRealSum(vol_p);
    ParallelDescriptor::ReduceRealSum(vol_s);

}
