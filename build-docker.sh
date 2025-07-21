#!/bin/bash
# EOPF-Zarr Docker Build and Test Script

set -e

echo "🐋 EOPF-Zarr Docker Build and Test Script"
echo "========================================"

# Configuration
IMAGE_NAME="eopf-zarr-driver"
TAG="latest"
FULL_IMAGE_NAME="$IMAGE_NAME:$TAG"

# Function to print colored output
print_step() {
    echo -e "\n🔵 $1"
}

print_success() {
    echo -e "✅ $1"
}

print_error() {
    echo -e "❌ $1"
}

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    print_error "Docker is not running. Please start Docker and try again."
    exit 1
fi

print_step "Building Docker image: $FULL_IMAGE_NAME"
docker build -t $FULL_IMAGE_NAME . || {
    print_error "Docker build failed!"
    exit 1
}

print_success "Docker image built successfully!"

print_step "Testing the image..."
docker run --rm $FULL_IMAGE_NAME python -c "
import sys
print(f'Python: {sys.version}')

from osgeo import gdal
gdal.AllRegister()
print(f'GDAL Version: {gdal.VersionInfo()}')
print(f'Total drivers: {gdal.GetDriverCount()}')

# Test EOPF-Zarr driver
driver = gdal.GetDriverByName('EOPFZARR')
if driver:
    print('✅ EOPF-Zarr driver loaded!')
    print(f'   Description: {driver.GetDescription()}')
else:
    print('⚠️ EOPF-Zarr driver not found')

# Test built-in Zarr driver
zarr_driver = gdal.GetDriverByName('Zarr')
if zarr_driver:
    print('✅ Built-in Zarr driver available')
else:
    print('❌ Built-in Zarr driver not found')
"

print_success "Image test completed!"

print_step "Image information:"
docker images | grep $IMAGE_NAME || echo "No image found"

echo -e "\n🚀 Build completed successfully!"
echo -e "\nNext steps:"
echo "1. 🧪 Test locally: docker-compose up"
echo "2. 🌐 Push to registry for JupyterHub use"
echo "3. 🔗 Configure JupyterHub at https://jupyterhub.user.eopf.eodc.eu"

echo -e "\n📋 Quick start commands:"
echo "  # Test locally"
echo "  docker-compose up"
echo ""
echo "  # Access JupyterLab"
echo "  open http://localhost:8888"
echo ""
echo "  # Stop containers"
echo "  docker-compose down"
