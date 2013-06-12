package main

// a package like "Go Lite IDE"
type Package struct {
	name string
	title string
}

func (p Package) String() string {
	return p.title + " (" + p.name + ")"
}
