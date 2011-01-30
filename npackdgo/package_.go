package main

type Package struct {
	name    string
	version string
}

func (p Package) String() string {
	return p.name + " " + p.version
}
