
#include "Geo.h"
namespace GCS{

Vector2D Line::CalculateNormal(Point &p, double* derivparam)
{
    Vector2D p1v(*p1.x, *p1.y);
    Vector2D p2v(*p2.x, *p2.y);

    Vector2D ret(0.0, 0.0);
    if(derivparam){
        if(derivparam==this->p1.x){
            ret.y += -1.0;
            //ret.x += 0;
        };
        if(derivparam==this->p1.y){
            //ret.y += 0;
            ret.x += 1.0;
        };
        if(derivparam==this->p2.x){
            ret.y += 1.0;
            //ret.x += 0;
        };
        if(derivparam==this->p2.y){
            //ret.y += 0;
            ret.x += -1.0;
        };
    } else {
        ret.x = -(p2v.y - p1v.y);
        ret.y = (p2v.x - p1v.x);
    };

    return ret;
}

Vector2D Circle::CalculateNormal(Point &p, double* derivparam)
{
    Vector2D cv (*center.x, *center.y);
    Vector2D pv (*p.x, *p.y);

    Vector2D ret(0.0, 0.0);
    if(derivparam){
        if (derivparam == center.x) {
            ret.x += -1;
            ret.y += 0;
        };
        if (derivparam == center.y) {
            ret.x += 0;
            ret.y += -1;
        };
        if (derivparam == p.x) {
            ret.x += +1;
            ret.y += 0;
        };
        if (derivparam == p.y) {
            ret.x += 0;
            ret.y += +1;
        };
    } else {
        ret.x = pv.x - cv.x;
        ret.y = pv.y - cv.y;
    };

    return ret;
}

Vector2D Ellipse::CalculateNormal(Point &p, double* derivparam)
{
    Vector2D cv (*center.x, *center.y);
    Vector2D f1v (*focus1X, *focus1Y);
    Vector2D pv (*p.x, *p.y);

    Vector2D ret(0.0, 0.0);

    Vector2D f2v ( 2*cv.x - f1v.x, 2*cv.y - f1v.y ); //position of focus2
    if(derivparam){
        //use numeric derivatives for testing
        double const eps = 0.00001;
        double oldparam = *derivparam;
        Vector2D v1 = this->CalculateNormal(p);
        *derivparam += eps;
        Vector2D v2 = this->CalculateNormal(p);
        *derivparam = oldparam;
        ret.x = (v2.x - v1.x) / eps;
        ret.y = (v2.y - v1.y) / eps;
    } else {
        Vector2D pf1 = Vector2D(pv.x - f1v.x, pv.y - f1v.y).getNormalized();
        Vector2D pf2 = Vector2D(pv.x - f2v.x, pv.y - f2v.y).getNormalized();
        ret.x = pf1.x + pf2.x;
        ret.y = pf1.y + pf2.y;
    };

    return Vector2D();
}

}//namespace GCS
