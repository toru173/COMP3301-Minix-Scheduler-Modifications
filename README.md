# Minix-Scheduler-Modifications
 This is a recreation of a project completed as part COMP3301 (Semester One, 2007). The assessment item requested that the scheduler for Minix 3 - a high availability POSIX compliant microkernel-based OS - is rewritten. More information about Minix can be obtained at http://minix3.org. The implimentation of the original assessment item seems to have been archived to an unknown location; however, I still have leture notes available.

Intel's Management Engine (or ME) is an SoC integrated into every modern Intel CPU. The software running on the ME is widely believed to be an OS derived from Minix (https://www.cs.vu.nl/~ast/intel/, http://blog.ptsecurity.com/2017/04/intel-me-way-of-static-analysis.html). Thus, it is an attractive OS for a researcher to target. The base code is well understood and highly modular; however, Intel's implimentation remains closed source.

Project status: Setting up development environment