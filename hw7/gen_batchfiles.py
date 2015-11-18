from string import Template

for n in [1000,1000062, 100006200]:
    for taskNo in [64,128,256,512,1024]:
        template=Template(
"""#!/bin/bash
#SBATCH -J particles        # Job Name
#SBATCH -o ${jobname}.o%j    # Output and error file name (%j expands to jobID)
#SBATCH -n ${taskNo}              # Total number of mpi tasks requested
#SBATCH -p normal          # Queue (partition) name -- normal, development, etc.
#SBATCH -t 00:05:00        # Run time (hh:mm:ss) - 5 minutes
#SBATCH -A TG-CCR150026

ibrun ./particles ${n}"""
        )
        jobname="MY"+str(taskNo) + "_doing_" + str(n)
        with open(jobname +".batch","w" as bf:
            bf.write(template.substitute(
                n=n,
                jobname=jobname,
                taskNo=taskNo
            ))
