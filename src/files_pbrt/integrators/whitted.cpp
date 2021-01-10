
/*
    pbrt source code is Copyright(c) 1998-2016
                        Matt Pharr, Greg Humphreys, and Wenzel Jakob.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */


// integrators/whitted.cpp*
#include "integrators/whitted.h"
#include "interaction.h"
#include "camera.h"
#include "film.h"
#include "paramset.h"
#include "materials/uber.h"
#include "materials/metal.h"
#include <fstream>
#include <fcntl.h>

namespace pbrt {

// WhittedIntegrator Method Definitions
Spectrum WhittedIntegrator::Li(const RayDifferential &ray, const Scene &scene,
                               Sampler &sampler, MemoryArena &arena,
                               int depth) const {
    Spectrum L(0.);
    // Find closest ray intersection or return background radiance
    SurfaceInteraction isect;
    if (!scene.Intersect(ray, &isect)) {
        for (const auto &light : scene.lights) L += light->Le(ray);
        return L;
    }

    // Compute emitted and reflected light at ray intersection point

    // Initialize common variables for Whitted integrator
    const Normal3f &n = isect.shading.n;
    Vector3f wo = isect.wo;

    // Compute scattering functions for surface interaction
    isect.ComputeScatteringFunctions(ray, arena);
    if (!isect.bsdf)
        return Li(isect.SpawnRay(ray.d), scene, sampler, arena, depth);

    // Compute emitted light if ray hit an area light source
    L += isect.Le(wo);

    // Add contribution of each light source
    for (const auto &light : scene.lights) {
        Vector3f wi;
        Float pdf;
        VisibilityTester visibility;
        Spectrum Li =
            light->Sample_Li(isect, sampler.Get2D(), &wi, &pdf, &visibility);
        if (Li.IsBlack() || pdf == 0) continue;
        Spectrum f = isect.bsdf->f(wo, wi);
        if (!f.IsBlack() && visibility.Unoccluded(scene))
            L += f * Li * AbsDot(wi, n) / pdf;

    }
    if (depth + 1 < maxDepth) {
        // Trace rays for specular reflection and refraction
        L += SpecularReflect(ray, isect, scene, sampler, arena, depth);
        L += SpecularTransmit(ray, isect, scene, sampler, arena, depth);
    }
    float amar_color[3];
    L.ToRGB(amar_color);

    const Material *m = isect.primitive->GetMaterial();

    //const UberMaterial *um = dynamic_cast<const UberMaterial*>(m);

    if( dynamic_cast<const UberMaterial*>(m))
    {
        const UberMaterial *um = dynamic_cast<const UberMaterial*>(m);
        //std::shared_ptr<Texture<Spectrum>> Kd = um->Kd;

        Spectrum s1 = um->Kd->Evaluate(isect);
        Spectrum s2 = um->Ks->Evaluate(isect);
        Spectrum s3 = um->Kr->Evaluate(isect);
        Spectrum s4 = um->Kt->Evaluate(isect);
        Spectrum s5 = um->roughness->Evaluate(isect);

        //std::ofstream material_props("./materials_props.txt",std::ios::out | std::ios::app);
        FILE* material_props = fopen("./materials_props.txt","a");
        int fd = fileno(material_props);
        flockfile(material_props);
        /*
        material_props << "***************" << "\n" << "|  Uber  |" << "\n" << "Kd :: " << s1.ToString() << "\n"
                                  << "Ks :: " << s2.ToString() << "\n" << "Kr :: " << s3.ToString() << "\n"
                                  << "Kt :: " << s4.ToString() << "\n" << "Roughness :: " << s5.ToString() << "\n"
                                  << "***************" << std::endl << std::endl;
        */

        fprintf(material_props, "%f, %f, %f, %s, %s, %s, %s, %s\n\n",
                isect.p.x, isect.p.y, isect.p.z, s1.ToString().c_str(), s2.ToString().c_str(),
                s3.ToString().c_str(), s4.ToString().c_str(), s5.ToString().c_str());

        funlockfile(material_props);
        fclose(material_props);
    }
    if( dynamic_cast<const MetalMaterial*>(m))
    {
        std::cout << "Metal" << std::endl;
    }

    //std::cout << "\nIntersection :: " << isect.bsdf->ToString() << " -- for -- " <<  isect.p.x << ", " << isect.p.y << ", " << isect.p.z << std::endl
    //<< std::endl;

    return L;
}

WhittedIntegrator *CreateWhittedIntegrator(
    const ParamSet &params, std::shared_ptr<Sampler> sampler,
    std::shared_ptr<const Camera> camera) {
    int maxDepth = params.FindOneInt("maxdepth", 5);
    int np;
    const int *pb = params.FindInt("pixelbounds", &np);
    Bounds2i pixelBounds = camera->film->GetSampleBounds();
    if (pb) {
        if (np != 4)
            Error("Expected four values for \"pixelbounds\" parameter. Got %d.",
                  np);
        else {
            pixelBounds = Intersect(pixelBounds,
                                    Bounds2i{{pb[0], pb[2]}, {pb[1], pb[3]}});
            if (pixelBounds.Area() == 0)
                Error("Degenerate \"pixelbounds\" specified.");
        }
    }

    return new WhittedIntegrator(maxDepth, camera, sampler, pixelBounds);
}

}  // namespace pbrt
