/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2010
 *     Mayte Lázaro, Alejandro R. Mosteo
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */


#include <cfloat>
#include <libplayercore/playercore.h>
#include "params.hh"
#include "robot_location.hh"

RobotLocation::RobotLocation(double odom_noise_x,
                             double odom_noise_y,
                             double odom_noise_th) :
    odom_noise_x_(odom_noise_x),
    odom_noise_y_(odom_noise_y),
    odom_noise_th_(odom_noise_th),
    XwRk_1(ROBOT),
    XwRk  (ROBOT),
    first_update_(true)
{
}

void RobotLocation::SetPoses(double ox, double oy, double oth,
                             double gx, double gy, double gth)
{
    const Transf global(gx, gy, gth);

    XwRk_1.SetLoc(global);
    XwRk.SetLoc(global);
    
    odomk_1 = Transf(ox, oy, oth);
}

void RobotLocation::SetCurrentError(double ex, double ey, double eth)
{

    Matrix cov = Matrix(3, 3);
    cov(0, 0) = pow(ex / 2, 2);
    cov(1, 1) = pow(ey / 2, 2);
    cov(2, 2) = pow(eth / 2, 2);

    XwRk_1.SetCov(cov);
    XwRk.SetCov(cov);
}

void RobotLocation::Prediction()
{

    double d, sx = odom_noise_x_, sy = odom_noise_y_, sphi = odom_noise_th_;

    const Transf Xrk_1rk = TRel(odomk_1, odomk);

    d = sqrt((Xrk_1rk.tX() * Xrk_1rk.tX()) +
             (Xrk_1rk.tY() * Xrk_1rk.tY()));

    Matrix Crk_1rk = Matrix(3, 3);
    Crk_1rk(0, 0)  = pow(sx * d, 2);
    Crk_1rk(1, 1)  = pow(sy * d, 2);
    Crk_1rk(2, 2)  = pow(sphi * Xrk_1rk.tPhi(), 2);

    const Matrix JRkRk_1 = InvJacobian(Xrk_1rk);

    XwRk.SetLoc(Compose(XwRk_1.kX(), Xrk_1rk));
    XwRk.SetCov(JRkRk_1 * XwRk_1.kCov() * ~JRkRk_1 + Crk_1rk);
}

///////////////////////////////////////////////////////////////
bool verifyOverlapping(Transf XME, double mLen, double eLen)
{
    return fabs(XME.tX()) <= ((mLen + eLen) / 2.0);
}

double MahalaDist(const Uloc &robot, const Feature &obs, const Transf &feat,
        Transf *Xme)
{
    Matrix Bme = Matrix(2, 3);
    Bme(0, 1)  = 1;
    Bme(1, 2)  = 1;

    const Transf Xre = obs.Loc();
    const Matrix Ce  = obs.Cov();

    const Transf &Xwm = feat;
    *Xme = TRel(Xwm, Compose(robot.kX(), Xre));

    // Compute h
    const Matrix h = Bme * (*Xme);

    // Compute HR
    const Matrix Jer = InvJacobian(Xre);
    const Matrix J2  = J2zero(*Xme);
    const Matrix HR  = Bme * J2 * Jer;

    // Compute GE
    const Matrix GE = Bme * J2 * ~Bme;

    // Hypothesis test -> Mahalanobis distance
    Matrix Cinn = !(HR * robot.kCov() * (~HR) + GE * Ce * (~GE));
    Matrix d2   = (~h) * Cinn * h;

    return d2(0, 0);
}

void UpdateWithMatch(Uloc *XwRk, const Feature &obs, const Transf &feat)
{
    Matrix Bme = Matrix(2, 3);
    Bme(0, 1)  = 1;
    Bme(1, 2)  = 1;

    const Transf Xre = obs.Loc();
    const Matrix Ce  = obs.Cov();

    const Transf &Xwm = feat;
    const Transf Xme = TRel(Xwm, Compose(XwRk->kX(), Xre));

    const Matrix h = Bme * Xme;

    const Matrix Jer = InvJacobian(Xre);
    const Matrix J2  = J2zero(Xme);
    const Matrix HR  = Bme * J2 * Jer;

    const Matrix GE = Bme * J2 * ~Bme;

    Matrix Cinn = !(HR * XwRk->kCov() * (~HR) + GE * Ce * (~GE));

    Matrix K  = XwRk->kCov() * ~HR * Cinn;
    Matrix Xk = -K * h;

    const Matrix P = XwRk->kCov() - K * HR * XwRk->kCov();

    //Centering
    XwRk->SetLoc(Compose(XwRk->kX(), Xk));
    XwRk->SetCov(InvJ2zero(Xk) * P * ~InvJ2zero(Xk));
}

void RobotLocation::Update(ObservedFeatures obs)
{
    int matched = 0;

    Matrix Bme = Matrix(2, 3);
    Bme(0, 1)  = 1;
    Bme(1, 2)  = 1;

    //  Match obs, in scan order, to the closest feature

    for (int i = 0; i < obs.Count(); i++)
    {
        int best_feat = -1;
        double best_dist = DBL_MAX;
        double best_xerr = DBL_MAX;

        for (int j = 0; j < map_.NumSegments(); j++)
        {
            Transf Xme;
            double dist;
            
            if ((dist = MahalaDist(XwRk, obs.features(i), map_.segments(j), &Xme)) <= 5.99)
                if (verifyOverlapping(Xme, map_.lengths(j), obs.features(i).dimension()))
                {
                    if (dist < best_dist)
                    {
                        best_dist = dist;
                        best_feat = j;
                        best_xerr = fabs(Xme.x());
                    }
                }
        }

        if (best_feat >= 0)
        {
            PLAYER_MSG7(4, "MATCH: %3d -- %3d [MAH:%8.3f][OVL:%8.3f/%8.3f/%8.3f/%8.3f]\n",
                i, best_feat, best_dist,
                best_xerr, obs.features(i).dimension(), map_.lengths(best_feat),
                best_xerr - ((obs.features(i).dimension() + map_.lengths(best_feat))/2.0));

            // Centering    
            UpdateWithMatch(&XwRk, obs.features(i), map_.segments(best_feat));
            
            matched++;
            GUI_DATA.matches.push_back(obs.features(i).GetScan());
            GUI_DATA.mahala.push_back(best_dist / 5.99);
        }
    }

    /* This is full quadratic search for the best pairing. Too expensive
    
    //  Best pairing is updated first. Pedro said this is best, until we have
    //    full Joint Compatibility
    std::vector<bool> used(obs.Count(), false);

    bool found;

    do
    {
        int best_obs  = -1;
        int best_feat = -1;
        double best_dist = DBL_MAX;
        double best_xerr = DBL_MAX;

        found = false;

        for (int i = 0; i < obs.Count(); i++)
        {
            if (!used[i])
                for (int j = 0; j < map_.NumSegments(); j++)
                {
                    Transf Xme;
                    double dist;
                    if ((dist = MahalaDist(XwRk, obs.features(i), map_.segments(j), &Xme)) <= 5.99)
                        if (verifyOverlapping(Xme, map_.lengths(j), obs.features(i).dimension()))
                        {
                            if (dist < best_dist)
                            {
                                best_dist = dist;
                                best_obs  = i;
                                best_feat = j;
                                best_xerr = fabs(Xme.x());
                            }
                        }
                }
        }

        if (best_obs >= 0)
        {
            found = true;
            used[best_obs] = true;
            
            printf("MATCH: %3d -- %3d [MAH:%8.3f][OVL:%8.3f/%8.3f/%8.3f/%8.3f]\n",
                best_obs, best_feat, best_dist,
                best_xerr, obs.features(best_obs).dimension(), map_.lengths(best_feat),
                best_xerr - ((obs.features(best_obs).dimension() + map_.lengths(best_feat))/2.0));

            // Centering    
            UpdateWithMatch(&XwRk, obs.features(best_obs), map_.segments(best_feat));
            
            matched++;
            GUI_DATA.matches.push_back(obs.features(best_obs).GetScan());
            GUI_DATA.mahala.push_back(best_dist / 5.99);
        }
        
    } while (found);

    return;

    */

    /* OLD STUMBLING METHOD:

    for (int i = 0; i < obs.Count(); i++)
    {
        const Transf Xre = obs.features(i).Loc();
        const Matrix Ce  = obs.features(i).Cov();
        
        for (int j = 0; j < map_.NumSegments(); j++)
        {
            const Transf Xwm = map_.segments(j);
            const Transf Xme = TRel(Xwm, Compose(XwRk.kX(), Xre));

            // Compute h
            const Matrix h = Bme * Xme;

            // Compute HR
            const Matrix Jer = InvJacobian(Xre);
            const Matrix J2  = J2zero(Xme);
            const Matrix HR  = Bme * J2 * Jer;

            // Compute GE
            const Matrix GE = Bme * J2 * ~Bme;

            // Hypothesis test -> Mahalanobis distance
            Matrix Cinn = !(HR * XwRk.kCov() * (~HR) + GE * Ce * (~GE));
            Matrix d2   = (~h) * Cinn * h;

            if (d2(0, 0) <= 5.99) // TODO name this constant
            {
                if (verifyOverlapping(Xme, map_.lengths(j), obs.features(i).dimension()))
                {
                    PLAYER_MSG3(4, "Set best match: %3d -- %3d (%8.3f)", i, j, d2(0, 0));
                    printf("MATCH: %3d -- %3d [MAH:%8.3f][OVL:%8.3f/%8.3f/%8.3f/%8.3f]\n",
                           i, j, d2(0, 0),
                           fabs(Xme.x()), obs.features(i).dimension(), map_.lengths(j),
                           fabs(Xme.x()) - ((obs.features(i).dimension() + map_.lengths(j))/2.0));
                    
                    Matrix K  = XwRk.kCov() * ~HR * Cinn;
                    Matrix Xk = -K * h;

                    const Matrix P = XwRk.kCov() - K * HR * XwRk.kCov();

                    //Centering
                    XwRk.SetLoc(Compose(XwRk.kX(), Xk));
                    XwRk.SetCov(InvJ2zero(Xk) * P * ~InvJ2zero(Xk));

                    matched++;
                    GUI_DATA.matches.push_back(obs.features(i).GetScan());
                    GUI_DATA.mahala.push_back(d2(0, 0) / 5.99);

                    break;
                }
                else if ((fabs(Xme.x()) - ((obs.features(i).dimension() + map_.lengths(j)))) <= 0)
                {
                    printf("NEAR : %3d -- %3d [MAH:%8.3f][OVL:%8.3f/%8.3f/%8.3f/%8.3f]\n",
                           i, j, d2(0, 0),
                           fabs(Xme.x()), obs.features(i).dimension(), map_.lengths(j),
                           fabs(Xme.x()) - ((obs.features(i).dimension() + map_.lengths(j))/2.0));
                }
                else
                    PLAYER_MSG2(6, "Pair %3d :: %3d not overlapping", i, j);
            }
            else if (d2(0, 0) < 100.0)
                PLAYER_MSG3(5, "Pair %3d :: %3d too far apart: %5.2f", i, j, d2(0, 0));
        }
    }

    */

    if (matched > 0)
        PLAYER_MSG1(2, "Ekfvloc: Matched feats: %d", matched);
    else
        PLAYER_WARN("Ekfvloc: No matching features!");
}

bool RobotLocation::Locate(const Transf odom, const Scan s)
{
//     if ((X(odom)   != X(odomk_1)) ||
//         (Y(odom)   != Y(odomk_1)) ||
//         (Phi(odom) != Phi(odomk_1)))

    if (first_update_ ||
        (odom.Distance(odomk_1) >= kMinOdomDistChange) ||
        (fabs(odom.tPhi() - odomk_1.tPhi()) >= kMinOdomAngChange))
    {
        first_update_ = false;
        odomk = odom;
        Prediction();

        //SEGMENTACION
        ObservedFeatures obs;
        ScanDataSegmentation(s, &obs);

        //Matching & EKF
        Update(obs);

        const Transf rel(TRel(odomk_1, odom));
        PLAYER_MSG3(8, "REL[1]: %8.3f %8.3f %8.3f\n", rel.tX(), rel.tY(), rel.tPhi());
        const Transf upd(TRel(XwRk_1.kX(), XwRk.kX()));
        PLAYER_MSG3(8, "FIX[1]: %8.3f %8.3f %8.3f\n", upd.tX(), upd.tY(), upd.tPhi());
        if (rel.Distance(upd) > 0.2)
            PLAYER_MSG0(2, "JUMP in localization");
        
        odomk_1 = odomk;
        XwRk_1  = XwRk;
        
        return true;
    }
    else
        return false;
}

void RobotLocation::PrintState() const
{
    cout << "Location: "    << ~XwRk.kX()   <<
            "Covariance:\n" <<  XwRk.kCov() << endl;
}

Pose RobotLocation::EstimatedPose(void) const
{
    return Pose(XwRk.kX().tX(),
                XwRk.kX().tY(),
                XwRk.kX().tPhi());
}

Matrix RobotLocation::Covariance(void) const
{
    return XwRk.kCov();
}
