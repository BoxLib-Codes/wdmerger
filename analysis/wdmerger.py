import numpy as np

def timing(output_filename):

    # Read in the file and return the following data in arrays:
    # - The coarse timestep time for each step
    # - The plotfile time for each step
    # For the sake of generality, we will count up how many
    # steps we have taken after the fact.

    output = open(output_filename, 'r')
    lines = output.readlines()
    coarseSteps = filter(lambda s: s[0:6] == "Coarse",lines)
    coarseSteps = [float(s.split()[3]) for s in coarseSteps] # Extract out the time only

    avg_timestep = np.average(coarseSteps)

    # Now subtract out the gravity solve time in each case

    grav_time = filter(lambda s: s[0:7] == "Gravity",lines)
    grav_time = [float(s.split()[3]) for s in grav_time]

    # Remove the first two, since they are related to the initial multilevel solve

    grav_time = grav_time[2:]

    # For each coarse timestep, there are two BC calculations and two Poisson solves. Let's
    # sum these for each timestep. For each refined level, there's two Poisson solves per subcycle.
    # For the coarse grid, there's two Poisson solves and also two BC fills.
    # Therefore, the number of gravity calculations per timestep is equal to
    # (nlevs - 1) * ref_ratio + 4

    ref_ratio = 4
    nlevs = 2

    grav_per_timestep = (nlevs - 1) * 2 * ref_ratio + 4

    for n in range(len(coarseSteps)):

        for i in range(grav_per_timestep):
            coarseSteps[n] -= float(grav_time[n+i])

    avg_timestep_no_grav = np.average(coarseSteps)

    return [avg_timestep, avg_timestep_no_grav]
