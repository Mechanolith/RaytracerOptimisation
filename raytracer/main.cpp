#include "omp.h"
#include "CImg.h"
#include "kf/kf_vector.h"
#include "scene.h"
#include "renderable.h"
#include "kf/kf_ray.h"
#include "light.h"
#include "windows.h"
#include "luascript.h"

using namespace cimg_library;

// This controls if the rendering displays progressively. When timing the results, turn this off (otherwise updating the window repeatedly will affect the timing)
// progressiveCount is how many lines are rendered before updating the window.
bool progressiveDisplay = false;
int  progressiveCount   = 10;


// The resolution of the window and the output of the ray tracer. This can be overridden by the Lua startup script.
int windowWidth = 1024;
int windowHeight = 1024;

// The scene object.
Scene g_scene;

// Lua state object used to run the startup script.
lua_State *g_state;



int main(int argc, char **argv)
{
	srand(0);
	
	std::string startupScript = "scene.lua";
	if (argc > 1)
	{
		startupScript = argv[1];
	}

	initLua(startupScript);

	
	// The floating point image target that the scene is rendered into.
	CImg<float> image(windowWidth, windowHeight, 1, 3, 0);
	// The display object used to show the image.
	CImgDisplay main_disp(image, "Raytrace");
	main_disp.set_normalization(0);	// Normalisation 0 disables auto normalisation of the image (scales to make the darkest to brightest colour fit 0 to 1 range.


	// Record the starting time.
	DWORD startTime = timeGetTime();

	// Primary loop through all screen pixels.
#pragma omp parallel for
	for (int y = 0; y < windowHeight; y++)
	{
	#pragma omp parallel for
		for (int x = 0; x < windowWidth; x+=3)
		{
			// Retrieve the colour of the specified pixel. The math below converts pixel coordinates (0 to width) into camera coordinates (-1 to 1).
			kf::Colour outputA = g_scene.trace(float(x - windowWidth / 2) / (windowWidth / 2), -float(y - windowHeight / 2) / (windowHeight / 2)*((float(windowHeight) / windowWidth)));
			kf::Colour outputC = g_scene.trace(float(x + 2 - windowWidth / 2) / (windowWidth / 2), -float(y - windowHeight / 2) / (windowHeight / 2)*((float(windowHeight) / windowWidth)));
			kf::Colour outputB;
			outputB.r = (outputA.r + outputC.r) / 2;
			outputB.g = (outputA.g + outputC.g) / 2;
			outputB.b = (outputA.b + outputC.b) / 2;

			
			// Clamp the output colour to 0-1 range before conversion.
			outputA.saturate();
			outputB.saturate();
			outputC.saturate();

			// Convert from linear space to sRGB.
			outputA.toSRGB();
			outputB.toSRGB();
			outputC.toSRGB();

			// Write the colour to the image (scaling up by 255).
			*image.data(x, y, 0, 0) = outputA.r*255;
			*image.data(x, y, 0, 1) = outputA.g*255;
			*image.data(x, y, 0, 2) = outputA.b*255;

			*image.data(x+1, y, 0, 0) = outputB.r * 255;
			*image.data(x+1, y, 0, 1) = outputB.g * 255;
			*image.data(x+1, y, 0, 2) = outputB.b * 255;

			*image.data(x+2, y, 0, 0) = outputC.r * 255;
			*image.data(x+2, y, 0, 1) = outputC.g * 255;
			*image.data(x+2, y, 0, 2) = outputC.b * 255;
		}

		// Perform progressive display if enabled.
		if (progressiveDisplay)
		{
			if (y % progressiveCount == 0)
			{
				main_disp.display(image);
				main_disp.set_title("Current render time: %dms", timeGetTime() - startTime);
			}
		}

		// Check for Escape key.
		//if (main_disp.is_keyESC())
			//return 0;
	}

	// Record ending time.
	DWORD endTime = timeGetTime();

	// Display elapsed time in the window title bar.
	main_disp.set_title("Render time: %dms", endTime - startTime);
	main_disp.display(image);

	// Save the output to a bmp file.
	image.save_bmp("output.bmp");
	// Keep refreshing the window until it is closed or escape is hit.
	while (!main_disp.is_closed())
	{
		if (main_disp.is_keyESC())
			return 0;
		main_disp.wait();
	}

	return 0;

}
