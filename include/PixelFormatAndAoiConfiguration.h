// Contains a configuration that sets pixel data format and Image AOI.

#ifndef INCLUDED_PIXELFORMATANDAOICONFIGURATION_H_00104928
#define INCLUDED_PIXELFORMATANDAOICONFIGURATION_H_00104928

#include <pylon/ConfigurationEventHandler.h>

namespace Pylon
{
    class CInstantCamera;
}
class CPixelFormatAndAoiConfiguration : public Pylon::CConfigurationEventHandler
{
public:
    void OnOpened( Pylon::CInstantCamera& camera)
    {
        try
        {
            // Allow all the names in the namespace GenApi to be used without qualification.
            using namespace GenApi;

            // Get the camera control object.
            INodeMap &control = camera.GetNodeMap();

            // Get the parameters for setting the image area of interest (Image AOI).
            const CIntegerPtr width = control.GetNode("Width");
            const CIntegerPtr height = control.GetNode("Height");
            const CIntegerPtr offsetX = control.GetNode("OffsetX");
            const CIntegerPtr offsetY = control.GetNode("OffsetY");

            // Maximize the Image AOI.
            if (IsWritable(offsetX))
            {
                offsetX->SetValue(offsetX->GetMin());
            }
            if (IsWritable(offsetY))
            {
                offsetY->SetValue(offsetY->GetMin());
            }
            width->SetValue(width->GetMax());
            height->SetValue(height->GetMax());

            // Set the pixel data format.
            CEnumerationPtr(control.GetNode("PixelFormat"))->FromString("Mono8");
        }
        catch (const GenericException& e)
        {
            throw RUNTIME_EXCEPTION( "Could not apply configuration. const GenericException caught in OnOpened method msg=%hs", e.what());
        }
    }
};

#endif /* INCLUDED_PIXELFORMATANDAOICONFIGURATION_H_00104928 */
