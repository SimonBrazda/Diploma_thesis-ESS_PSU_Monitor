// A Module for Power Supply Analysis of Electronic Security Systems
// Copyright (C) 2021 by Šimon Brázda <simonbrazda@seznam.cz>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/**************
TFT_eSPI driver for arduino menu
turning TFT_eSPI data into a menu input stream
checks menu option clicked or do menu scroll on drag
***/

#pragma once

#include <menuIO/TFT_eSPIOut.h>
#include <TFT_eSPI.h>
#include <menuDefs.h>

namespace Menu {

    class TFT_eSPI_touchIn:public menuIn {
    public:
        TFT_eSPI& touch;
        navRoot& root;
        TFT_eSPIOut& out;
        uint16_t startX,startY,z;
        int scrlY;
        bool touching;
        bool dragging;
        unsigned long evTime;
        TFT_eSPI_touchIn(TFT_eSPI& t,navRoot& root,TFT_eSPIOut& out):touch(t),root(root),out(out),touching(false),dragging(false) {}
        int available(void) {
            return touch.getTouch(&startX,&startY);
        }
        int peek(void) {return -1;}
        int read() {
            menuNode& m=root.active();
            panel p=out.panels[out.panels.cur];
            if (touch.getTouch(&startX,&startY)) {
                if (root.sleepTask) return options->navCodes[enterCmd].ch;
                evTime=millis();
                // touch.getTouch(&startX,&startY);
                z = touch.getTouchRawZ();
                // Serial.println("x: " + String(startX) + ", y: " + String(startY) + ", z: " + String(z));
                // startX=touch.getX()-p.x*out.resX;
                // Serial.println("x: " + String(touch.getX()));
                if (startX>out.maxX()*out.resX) return -1;
                int y=startY-p.y*out.resY;//-menuNode::activeNode->oy;
                // Serial.println("y: " + String(touch.getY()));
                if (y<0||y>out.maxY()*out.resY) return -1;
                // Serial.println("x: " + String(startX) + ", y: " + String(y));
                //within menu box
                if (touching) {//might be dragging
                    // Serial.println("touching");
                    int d=scrlY-y;
                    int ad=abs(d);
                    if (ad>(out.resY>>1)&&(ad<<1)>out.resY) {
                        dragging=true;//we consider it a drag
                        // Serial.println("dragging");
                        scrlY-=(d>0?1:-1)*(out.resY/2);
                        //prompt* nf=root.navFocus;
                        /*if (nf->isMenu()&&!nf->isVariant()) {
                            MENU_DEBUG_touch<<"utouch scrolling "<<(d>0?"Up":"Down")<<endl;
                            return (d>0?options->navCodes[scrlUpCmd].ch:options->navCodes[scrlDownCmd].ch);
                        } else {*/
                            //MENU_DEBUG_touch<<"utouch moving "<<(d>0?"Up":"Down")<<endl;
                            return (d>0?options->navCodes[upCmd].ch:options->navCodes[downCmd].ch);
                        //}
                    }
                } else {//start new touching
                    touching=true;
                    dragging=false;
                    startY=y;
                    scrlY=y;
                    evTime=millis();
                }
            } else {
                if (millis()-evTime<200) return -1;//debouncing
                touching=false;//touch ending
                if (dragging) return -1;
                int st=root.showTitle?1:0;
                if (root.navFocus->isMenu()&&!root.navFocus->parentDraw()) {
                    int at=startY/out.resY;
                    //MENU_DEBUG_touch<<"utouch index select "<<((at>=st&&at<(m.sz()+st))?at-st+touch.top(root.node())+'1':-1)<<endl;
                    //MENU_DEBUG_touch<<"canNav:"<<root.navFocus->canNav()<<"isVariant:"<<root.navFocus->isVariant()<<endl;
                    return (at>=st&&at<(m.sz()+st))?at-st+out.top(root.node())+'1':-1;
                } else {//then its some sort of field
                    prompt& a=m;//root.active();
                    //MENU_DEBUG_touch<<"utouch "<<(memStrLen(a.shadow->text)*touch.resX<startX?"enter":"escape")<<endl;
                    return
                    ((int)(memStrLen(a.getText())*out.resX))<startX?
                        options->navCodes[enterCmd].ch:
                        options->navCodes[escCmd].ch;
                }
            }
            return -1;
        }
        void flush() {}
        size_t write(uint8_t v) {return 0;}
    };

}//namespace Menu