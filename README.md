## Useage
- Based on openmp. You may compile the source code with gcc/icc/clang/icl. icc/icl bring about 4 times speed-up to this program.
- On the Xsede (https://portal.xsede.org/my-xsede#/guest) platform, this program achieves the following performance, in terms of searching extremum of example function in interval [1, 100]:
	- 4 cores, 1.52s
	- 16 cores, 0.77s
	- 28 cores, 0.64s
- For any continuous and derivatived function $f(x)$, you may reply on this program to find the extremum in seconds, given the following info:
	- interval [a, b]
	- bound on the absolute value of derivative s
	- bound on the error of extremum $\epsilon$
## Method
In short, devide and conquer. First, we divide the initial interval [a, b] into two sub-intervals, and check the value in boundary to update the extremum. Then compute the possible extremum (f(a) + f(b) + s(b-a))/2 into two sub-intervals according to s. If the possible extremum is smaller than current extremum, we drop the interval, otherwise, repeat two steps recursively, until the interval span is smaller than $\epsilon$. 
Since Xsede provides many cores, we can process many valid intervals at the same time. At the very begining, in order to aviod too many idle cores, we generate several intervals and store them in a shared queue (master), then each core (slave) take out interval from queue and search it one by one. When all intervals in master queue are processed, the extremum is found.
