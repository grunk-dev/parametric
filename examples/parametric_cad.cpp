// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

#include <parametric/core.hpp>

#include <TopoDS_Shape.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <BRepAlgoAPI_Cut.hxx>

#include <iostream>

class CylinderParms
{
public:
    CylinderParms(double w, double h)
        : _w(w), _h(h) {}

    void SetW(double w) {_w = w;}
    void SetH(double h) {_h = h;}

    double W() const {return _w;}
    double H() const {return _h;}

private:
    double _w, _h;
};

// create a box, just with plain old doubles
TopoDS_Shape makeBox (double a, double b, double c)
{
    std::cout << "  Compute box" << std::endl;
    return BRepPrimAPI_MakeBox(gp_Pnt(-a/2., -b/2., .0), a, b, c).Shape();
}

int main()
{
    // Define the cylinder parameters
    parametric::param<CylinderParms> cyl_parms(CylinderParms(3, 10), "p1");

    // Create a cylinder, use custom parametrization object, we can use a lambda
    parametric::param<TopoDS_Shape> cyl = parametric::eval([](const CylinderParms& parms) {
        std::cout << "  Compute cylinder" << std::endl;
        return BRepPrimAPI_MakeCylinder(parms.W()/2., parms.H()).Shape();
    }, cyl_parms);

    // Define the shape parameters of the box
    auto box_w = parametric::new_param(10., "w");
    auto box_l = parametric::new_param(10., "l");
    // optionally, we can also omit the id
    auto box_h = parametric::new_param(10.);

    // Of course, we also can use auto as a return type
    // Instead of using a lambda, lets use a normal function
    auto box = parametric::eval(makeBox, box_w, box_l, box_h);

    // define boolean cut free function
    auto boolean_cut = [](const TopoDS_Shape& s1, const TopoDS_Shape& s2) {
        std::cout << "  Compute boolean  cut" << std::endl;
        return BRepAlgoAPI_Cut(s1, s2).Shape();
    };

    // Perform the cut. The result is a hole in the cylinder.
    auto result = parametric::eval(boolean_cut, box, cyl);

    std::cout << "Writing result" << std::endl;
    BRepTools::Write(result, "result1.brep");

    std::cout << "change width of cylinder -> cylinder must be recomputed" << std::endl;
    cyl_parms.change_value().SetW(8);
    BRepTools::Write(result, "result2.brep");

    std::cout << "change width of box -> box must be recomputed" << std::endl;
    box_w = 20.;
    BRepTools::Write(result, "result3.brep");

    std::cout << "nothing has changed so no recomputation is required" << std::endl;
    BRepTools::Write(result, "result4.brep");
}
