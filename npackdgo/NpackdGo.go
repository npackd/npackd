package main

import (
	"fmt"
)

// main entry point
func main() {
	packages := make([]*Package, 0, 100)
	packageVersions := make([]*PackageVersion, 0, 100)

	p := new(Package)
	p.title = "Test package"
	p.name = "test"
	packages = append(packages, p)
	
	pv := new(PackageVersion)
	pv.Package = p.name
	// 
	pv.Version = Version{[...]int{1,2,3}[:]}
	packageVersions = append(packageVersions, pv)

	fmt.Println("Packages:")
	for _, p = range packages {
		fmt.Println(p)
	}

	fmt.Println("Package versions:")
	for _, pv = range packageVersions {
		fmt.Println(pv)
	}
}
