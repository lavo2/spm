To run on local machine:

mpirun -n #nprocess #source

mpirun -n 3 ./hello_world



to compile code on a specific node
mpirun -w #nodename make #make_command



# to allocate N node on the machine

salloc -N #n --time=#MINUTES


# to check which nodes are available

sinfo

# to run mpi on different nodes

mpirun -n 4 --host node01,node02,node03,node04 ./program

# with slurm
srun --mpi=pmix -N 4 ./hello_world

# with hostfile file (it uses all the resourced of every node by default)

mpirun -n 4 -hostifile hostfile ./hello_world

# to force node splitting before resources (like core for each node)

mpirun -n 4 -hostifile hostfile -bynode ./hello_world



# SLURM

# to check if am in queue for something

squeue --me

# to cancel and free resources (the id is taken from squeue --me )

scancel #id


