/*
 * Copyright (c) 2002-2006 John M. Fregeau, Richard Campbell, Jeff Molofee
 * Copyright (c) 2011 Matthew Arsenault
 *
 * This file is part of Milkway@Home.
 *
 * Milkyway@Home is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Milkyway@Home is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _NBODY_GL_H_
#define _NBODY_GL_H_

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#ifndef _WIN32
  #include <pthread.h>
#endif /* _WIN32 */

#include "nbody.h"
#include "nbody_types.h"

#ifndef _WIN32
typedef pthread_t NBodyThreadID;
#else
typedef HANDLE NBodyThreadID;
#endif /* _WIN32 */



int nbodyGLSetup(int* argc, char** argv);
int nbodyInitDrawState(const NBodyState* st);
void nbodyGLCleanup();
void updateDisplayedBodies();
void nbodyRunDisplayWhenReady();
int runNBodySimulationInThread(NBodyFlags* nbf, NBodyThreadID* tid);
int nbodyWaitForSimThread(NBodyThreadID tid);

#endif /* _NBODY_GL_H_ */

