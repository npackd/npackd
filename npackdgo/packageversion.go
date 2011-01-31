package main

type PackageVersion struct {
	package_ string
	version string
}

func (p PackageVersion) String() string {
	return p.package_ + " " + p.version
}
