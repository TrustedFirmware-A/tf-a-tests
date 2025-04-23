Introduction to SMC Fuzzer
==========================

The SMC fuzzer is a tool designed to enhance testing capability while
giving the user the ability to discover bugs more efficiently.  It is
ideally used after an initial phase of directed testing as given by tf-a-tests.
The primary mechanism for exercising the code base is the library of
SMC calls used to call into the various features sets of Trusted Firmware-A.
The user derives the list of SMC calls and then designs the arguments
for each to be submitted to the fuzzer as a SMC definition file. The
fuzzer functions optimally when the SMC calls are not dependent on order
or sequencing.
