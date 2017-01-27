#include <Ogre.h>
#pragma once

// here we include the header of Magic Library
//#include "OgreMagic.h"

using namespace Ogre;

class MagicRenderQueue:public Ogre::RenderQueueListener
{
   private:
         // Here we declare our variable
//         HSPRITE rastaImage;
    //     HSPRITE haloImage;
     //    HSPRITE Surface;

   public:

        MagicRenderQueue(RenderWindow*    mWin, String path)
         {
		 }
             // Here we initializing our variable
/*
             if(ML_FileExist("MagicMedia.cfg"))
                ML_SetMediaDirectoryFromFile("MagicMedia.cfg");
             else
                ML_SetMediaDirectory((char*)path.c_str());

             Surface = ML_CreateSprite(570,100);
             ML_StartDrawingToSprite(Surface);
             cairo_t* cr = ML_GetCairoCr();
             cairo_select_ft_font(cr, ML_GetWindowsPath("fonts/tahomabd.ttf"));
             cairo_set_font_size(cr, 90.0);
             cairo_move_to(cr, 50, 80);
             cairo_text_path(cr, "OgerMagic");
             cairo_set_source_rgb(cr, 0, 0, 0);
             cairo_fill_preserve(cr);
             cairo_set_source_rgb(cr, 1, 1, 1);
             cairo_set_line_width(cr, 2.50);
             cairo_stroke(cr);
             ML_StopDrawingToSprite(Surface);

             haloImage = ML_LoadSprite("halo.bmp");
             ML_SetColorKeyFromPoint(haloImage,0,0);

             rastaImage = ML_CreateSprite(1,64);
             ML_StartDrawingToSprite(rastaImage);
               cr = ML_GetCairoCr();
               cairo_pattern_t *pat = cairo_pattern_create_linear(0,0,0,64);
               cairo_pattern_add_color_stop_rgba(pat,  0, 0, 0, 0 ,0);
               cairo_pattern_add_color_stop_rgba(pat,0.5, 1, 1, 1 ,1);
               cairo_pattern_add_color_stop_rgba(pat,  1, 0, 0, 0 ,0);
               cairo_set_source(cr, pat);
               cairo_rectangle(cr, 0, 0, 1,64);
               cairo_fill(cr);
               cairo_pattern_destroy(pat);
             ML_StopDrawingToSprite(rastaImage);

         }

        virtual ~MagicRenderQueue()
         {

         }

        /// Called by Ogre, for being a render queue listener
        virtual void renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation)
         {
           // very important
           // first check the invocation  and queueGroupId parameters
           if(  queueGroupId == RENDER_QUEUE_OVERLAY && invocation != "SHADOWS")
            {
                    ML_UpdateTimer();
                    ML_Use2DView();
                    ML_SetAlpha(1.0);
                    ML_SetColor(255,255,255);

                    ML_SetBlendMode(LIGHTBLEND);

                    static float midWidth  = ML_GetScreenWidth()/2.0;
                    static float midHeight = ML_GetScreenHeight()/2.0;

                    static float angle=0,angleadd=7,freq=1;
                    static float angle1=0,angleadd1=5,freq1=1.2;
                    static float angle2=0,angleadd2=3,freq2=1.65;
                    static float theta;


                    if(ML_Every(50))
                     {
                         angle +=angleadd;
                         angle1+=angleadd1;
                         angle2+=angleadd2;
                         theta = (theta >= 360-1.5) ? 0 : theta + 0.01;
                     }

                    ML_SetScale(1.0,1.7);
		            ML_SetColor(0,0,255);
                    float xstart=midWidth-256,ycenter=midHeight,scale=28;
		            for(float i=0;i<512;i++)
		             {
			            float temp = i+xstart;
			            if(temp>0 || temp<512)
				          ML_DrawSprite(rastaImage, temp, ycenter + sin((i+angle)*freq*3.141592654/180.0)*scale);
		     	     }

		     	    ML_SetColor(0,255,0);
		     	    xstart=midWidth-256+5,ycenter=midHeight,scale=28;
                    for(float i=0;i<512;i++)
		             {
			           float  temp = i+xstart;
			           if(temp>0 || temp<512)
				          ML_DrawSprite(rastaImage, temp, ycenter + sin((i+angle1)*freq1*3.141592654/180.0)*scale);
		     	     }

                    ML_SetColor(255,0,0);
		     	    xstart=midWidth-256+10,ycenter=midHeight,scale=28;
                    for(float i=0;i<512;i++)
		             {
			           float  temp = i+xstart;
			           if(temp>0 || temp<512)
				         ML_DrawSprite(rastaImage, temp, ycenter + sin((i+angle2)*freq2*3.141592654/180.0)*scale);
		     	     }

                    ML_DrawSprite(Surface,midWidth,midHeight);

                    ML_SetAlpha(0.2);
                    ML_SetColor(255,0,0);
                    ML_DrawSprite(haloImage,sin(theta)*200+midWidth,cos(theta/2.0)*200+midHeight);
                    ML_DrawSprite(haloImage,-sin(theta)*200+midWidth,cos(theta/2.0)*200+midHeight);
                    ML_SetColor(0,255,0);
                    ML_DrawSprite(haloImage,sin(theta+1)*200+midWidth,cos(theta/2.0+1)*200+midHeight);
                    ML_DrawSprite(haloImage,-sin(theta+1)*200+midWidth,cos(theta/2.0+1)*200+midHeight);
                    ML_SetColor(0,0,255);
                    ML_DrawSprite(haloImage,sin(theta-1)*200+midWidth,cos(theta/2.0-1)*200+midHeight);
                    ML_DrawSprite(haloImage,-sin(theta-1)*200+midWidth,cos(theta/2.0-1)*200+midHeight);

                    ML_Stop2DView();

            }
         }

        /// Called by Ogre, for being a render queue listener
        virtual void renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation)
         {
              if(  queueGroupId == RENDER_QUEUE_OVERLAY && invocation != "SHADOWS")
               {


               }

         }*/
};
