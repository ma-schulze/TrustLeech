# Infrastructure

To allow easier reproduction of the described claims as part of the ACSAC artifact evaluation, we provide a `CloudLab` profile with all requirements installed that may be used as an evaluation platform. 

## Saved Disk Image URN
urn:publicid:IDN+utah.cloudlab.us+image+trustleech-PG0:TrustLeech-small-lan

## Saved Profile
https://www.cloudlab.us/p/TrustLeech/TrustLeech-ACSAC-AE

## Running 
Even though specified that user groups have been changed, the saved disk images did not seem to include these changes.
Therefore, on first use, you need to execute the following commands:
```
sudo usermod -aG docker $USER 
newgrp docker
```
