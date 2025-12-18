# Pre-requisite

cmake installed. 
```bash
brew install cmake@3.30.5
brew install qt@6.9.3
```

# Build 
```bash
mkdir build # create build directory
cd build # navigate to this directory
cmake .. # initialie project.
make # creates executable image-processing in path build/image-processing.app/Contents/MacOS/image-processing
```
`
# Run
```bash
./image-processing.app/Contents/MacOS/image-processing # this will execute the file (make sure to run in when inside build or ./build/image-processing.app/Contents/MacOS/image-processing if in root dir
```
