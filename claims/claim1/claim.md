# Claim 1 - Performance 

This test case evaluates the performance claims made in Section 5.1 of the paper. 

For this, it exercises four test cases. 
After these test cases, the generated output is grepped for the wanted values, which are then output into a *claim1_results.txt* file.

## Testcase 1. Injection Slowdown
First, this test case evaluates the performance costs of injecting the TrustLeech Hypervisor. 
The time is measured by the kernel module used to initialize the injection.

## Testcase 2. Runtime Slowdown
This test case evaluates the observable slowdown of the Hypervisor and the VM if TrustLeech is active. 
For this, before Testcase 1., *CoreMark-Pro* is excuted on both the Hypervisor and a VM. 

Testcase 2. then repeats these tests with the now active TrustLeech Hypervisor. 
The results of the respective runs are then compared.

## Testcase 3. Removal Slowdown 
First, this test case evaluates the performance costs of removing the active TrustLeech Hypervisor. 
The time is measured by the kernel module used to initialize the removal.


