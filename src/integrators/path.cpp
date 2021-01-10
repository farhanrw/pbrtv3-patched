
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

// integrators/path.cpp*
#include <materials/uber.h>
#include <materials/glass.h>
#include <materials/mirror.h>
#include <materials/plastic.h>
#include <materials/matte.h>
#include <materials/metal.h>
#include <materials/substrate.h>

#include "integrators/path.h"
#include "bssrdf.h"
#include "camera.h"
#include "film.h"
#include "interaction.h"
#include "paramset.h"
#include "spectrum.h"
#include "scene.h"
#include "stats.h"
#include <sys/file.h>
#include <string>
#include "textures/constant.h"

bool RENDER_MODE_INFERENCE = false;
bool writeFaces = false;
bool greenRender = false;

namespace pbrt {

    STAT_PERCENT("Integrator/Zero-radiance paths", zeroRadiancePaths, totalPaths);
    STAT_INT_DISTRIBUTION("Integrator/Path length", pathLength);

// PathIntegrator Method Definitions
    PathIntegrator::PathIntegrator(int maxDepth,
                                   std::shared_ptr<const Camera> camera,
                                   std::shared_ptr<Sampler> sampler,
                                   const Bounds2i &pixelBounds, Float rrThreshold,
                                   const std::string &lightSampleStrategy)
            : SamplerIntegrator(camera, sampler, pixelBounds),
              maxDepth(maxDepth),
              rrThreshold(rrThreshold),
              lightSampleStrategy(lightSampleStrategy) {}

    void PathIntegrator::Preprocess(const Scene &scene, Sampler &sampler) {
        lightDistribution =
                CreateLightSampleDistribution(lightSampleStrategy, scene);
    }

    Spectrum PathIntegrator::Li(const RayDifferential &r, const Scene &scene,
                                Sampler &sampler, MemoryArena &arena,
                                int depth) const {
        ProfilePhase p(Prof::SamplerIntegratorLi);
        Spectrum L(0.f), beta(1.f);
        Point2i potter;

        RayDifferential ray(r);
        bool specularBounce = false;
        int bounces;
        // Added after book publication: etaScale tracks the accumulated effect
        // of radiance scaling due to rays passing through refractive
        // boundaries (see the derivation on p. 527 of the third edition). We
        // track this value in order to remove it from beta when we apply
        // Russian roulette; this is worthwhile, since it lets us sometimes
        // avoid terminating refracted rays that are about to be refracted back
        // out of a medium and thus have their beta value increased.
        Float etaScale = 1;

        for (bounces = 0;; ++bounces)
        {
            // Find next path vertex and accumulate contribution
            VLOG(2) << "Path tracer bounce " << bounces << ", current L = " << L
                    << ", beta = " << beta;

            // Intersect _ray_ with scene and store intersection in _isect_
            SurfaceInteraction isect;
            bool foundIntersection = scene.Intersect(ray, &isect);

            // ---- ADD CODE FOR OUTPUT GENERATION
            if(bounces==0 && foundIntersection && !specularBounce && writeFaces)
            {
                FILE *face_writer = fopen("./face_writer.txt", "a");
                int fw = fileno(face_writer);

                flock(fw, LOCK_EX);
                int x = sampler.currentPixel.x;
                int y = sampler.currentPixel.y;
                int sample_number = sampler.currentPixelSampleIndex;

                if(isect.shape != nullptr)
                {
                    int sid = isect.shape->shape_id;
                    int pid = isect.shape->parent_id;
                    Vector3f u_data = Normalize(isect.dpdu);
                    Vector3f v_data = Normalize(isect.dpdv);
                    fprintf(face_writer, "%d, %d, %d, %d, %f, %f, %f, %f, %f, %f\n", x, y, sid, pid,
                            isect.p[0], isect.p[1], isect.p[2], isect.n[0], isect.n[1], isect.n[2]
                    );
                }

                flock(fw, LOCK_UN);
                fclose(face_writer);
            }
            int x = sampler.currentPixel.x;
            int y = sampler.currentPixel.y;
            int sample_number = sampler.currentPixelSampleIndex;


//            if (bounces == 0 && foundIntersection )
//            {
//
//                Primitive* prim = isect.primitive;
//                GeometricPrimitive *gm = dynamic_cast<GeometricPrimitive *>(prim);
//                Material *holder = gm->material.get();
//
//                if (true)
//                {
//                    FILE* pFile = fopen ("/home/farhan/Farhan_Thesis_Codes/pbrt-v3/build/somefile.txt","r");
//
//                    if(pFile == NULL)
//                    {
//                        pFile = fopen ("/home/farhan/Farhan_Thesis_Codes/pbrt-v3/build/somefile.txt","r");
//
//                        if(pFile == NULL)
//                        {
//                            std::cout << x << ", " << y << std::endl;
//                            std::cout << "BREAKING -------------------------\n";
//                            return 0;
//                        }
//                    }
//                    float kd1, kd2, kd3;
//                    float ks1, ks2, ks3;
//                    float kr1, kr2, kr3;
//                    float kt1, kt2, kt3;
//                    int xp, yp;
//                    float rough;
//
//                    do
//                    {
//                        fscanf (pFile, "%d, %d, %f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
//                                &xp, &yp, &kd1, &kd2, &kd3, &ks1, &ks2, &ks3, &kr1, &kr2, &kr3, &kt1, &kt2, &kt3,
//                                &rough );
//                    }
//                    while((xp != x || yp != y) && !feof(pFile));
//                    fclose(pFile);
//                    //std::cout << "Found " << xp << ", " << yp << " for " << x << ", " << y << " ::  " << kd1 << ", " <<
//                    //        kd2 << ", " << kd3 << std::endl;
//
//                    RGBSpectrum Kd_spectrum;
//                    Float Kd_values[3];
//                    Kd_values[0] = kd1;
//                    Kd_values[1] = kd2;
//                    Kd_values[2] = kd3;
//                    Kd_spectrum = Kd_spectrum.FromRGB(Kd_values);
//
//                    RGBSpectrum Ks_spectrum;
//                    Float Ks_values[3];
//                    Ks_values[0] = ks1;
//                    Ks_values[1] = ks2;
//                    Ks_values[2] = ks3;
//                    Ks_spectrum = Ks_spectrum.FromRGB(Ks_values);
//
//                    RGBSpectrum Kr_spectrum;
//                    Float Kr_values[3];
//                    Kr_values[0] = kr1;
//                    Kr_values[1] = kr2;
//                    Kr_values[2] = kr3;
//                    Kr_spectrum = Kr_spectrum.FromRGB(Kr_values);
//
//                    RGBSpectrum Kt_spectrum;
//                    Float Kt_values[3];
//                    Kt_values[0] = kt1;
//                    Kt_values[1] = kt2;
//                    Kt_values[2] = kt3;
//                    Kt_spectrum = Kt_spectrum.FromRGB(Kt_values);
//
//                    std::shared_ptr<ConstantTexture<Spectrum>> Kd = std::make_shared<ConstantTexture<Spectrum>>(Kd_spectrum);
//                    std::shared_ptr<ConstantTexture<Spectrum>> Ks = std::make_shared<ConstantTexture<Spectrum>>(Ks_spectrum);
//                    std::shared_ptr<ConstantTexture<Spectrum>> Kr = std::make_shared<ConstantTexture<Spectrum>>(Kr_spectrum);
//                    std::shared_ptr<ConstantTexture<Spectrum>> Kt = std::make_shared<ConstantTexture<Spectrum>>(Kt_spectrum);
//                    std::shared_ptr<ConstantTexture<Float>> roughness = std::make_shared<ConstantTexture<Float>>(rough);
//                    std::shared_ptr<ConstantTexture<Float>> ru = std::make_shared<ConstantTexture<Float>>(0.0);
//                    std::shared_ptr<ConstantTexture<Float>> rv = std::make_shared<ConstantTexture<Float>>(0.0);
//                    std::shared_ptr<ConstantTexture<Spectrum>> opacity = std::make_shared<ConstantTexture<Spectrum>>(1);
//                    std::shared_ptr<ConstantTexture<Float>> eta = std::make_shared<ConstantTexture<Float>>(1.5);
//                    std::shared_ptr<ConstantTexture<Float>> bumpmap = std::make_shared<ConstantTexture<Float>>(0.0);
//                    std::shared_ptr<ConstantTexture<bool>> remap = std::make_shared<ConstantTexture<bool>>(1);
//
//                    UberMaterial *new_uber = new UberMaterial(Kd, Ks, Kr, Kt, roughness,ru, rv, opacity, eta, bumpmap,
//                                                              true);
//
//                    //std::shared_ptr<Material> main_man;// = std::make_shared<Material>(new_uber);
//                    gm->material = std::make_shared<UberMaterial>(*new_uber);
//
//                }
//
//                const Material *m = isect.primitive->GetMaterial();
//
//
//
//                if (dynamic_cast<const UberMaterial *>(m)) {
//                    const UberMaterial *um = dynamic_cast<const UberMaterial *>(m);
//                    //std::shared_ptr<Texture<Spectrum>> Kd = um->Kd;
//
//                    Spectrum s1 = um->Kd->Evaluate(isect);
//                    Spectrum s2 = um->Ks->Evaluate(isect);
//                    Spectrum s3 = um->Kr->Evaluate(isect);
//                    Spectrum s4 = um->Kt->Evaluate(isect);
//                    Spectrum s5 = um->roughness->Evaluate(isect);
//                    Spectrum s6 = um->eta->Evaluate(isect);
//                    Spectrum s7 = um->opacity->Evaluate(isect);
//                    Spectrum s8, s9;
//                    float remap = um->remapRoughness;
//
//                    if(um->roughnessu == nullptr)
//                    {
//                        s8 = Spectrum(0.0f);
//                    }
//                    else
//                    {
//                        s8 = um->roughnessu->Evaluate(isect);
//                    }
//                    if(um->roughnessv == nullptr)
//                    {
//                        s9 = Spectrum(0.0f);
//                    }
//                    else
//                    {
//                        s9 = um->roughnessv->Evaluate(isect);
//                    }
//
////                FILE *material_props = fopen("./materials_props.txt", "a");
////                int fd = fileno(material_props);
////                flock(fd, LOCK_EX);
////
////
////                fprintf(material_props, "%d, %d, %" PRId64 ", %f, %f, %f, %s, %s, %s, "
////                                        "%s, %s, %s, %s, %s, %s, %f\n",
////                        x, y, sample_number, isect.p.x, isect.p.y, isect.p.z, s1.ToString().c_str(),
////                        s2.ToString().c_str(), s3.ToString().c_str(), s4.ToString().c_str(),
////                        s5.ToString().c_str(), s6.ToString().c_str(), s7.ToString().c_str(),
////                        s8.ToString().c_str(), s9.ToString().c_str(), remap
////                        );
////
////                flock(fd, LOCK_UN);
////                fclose(material_props);
//                }
//
//                if (dynamic_cast<const MetalMaterial *>(m)) {
//                    const MetalMaterial *mm = dynamic_cast<const MetalMaterial *>(m);
//                    //std::shared_ptr<Texture<Spectrum>> Kd = um->Kd;
//
//                    Spectrum s1 = mm->roughness->Evaluate(isect);
//                    Spectrum s2 = mm->eta->Evaluate(isect);
//                    std::string default_string = " [ 0.0, 0.0, 0.0 ]";
//
//                    Spectrum s8, s9;
//                    float remap = mm->remapRoughness;
//
//                    if(mm->uRoughness == nullptr)
//                    {
//                        s8 = Spectrum(0.0f);
//                    }
//                    else
//                    {
//                        s8 = mm->uRoughness->Evaluate(isect);
//                    }
//                    if(mm->vRoughness == nullptr)
//                    {
//                        s9 = Spectrum(0.0f);
//                    }
//                    else
//                    {
//                        s9 = mm->vRoughness->Evaluate(isect);
//                    }
//
////                FILE *material_props = fopen("./materials_props.txt", "a");
////                int fd = fileno(material_props);
////                flock(fd, LOCK_EX);
////
////                fprintf(material_props, "%d, %d, %" PRId64 ", %f, %f, %f, %s, %s, %s, "
////                                        "%s, %s, %s, %s, %s, %s, %f\n",
////                        x, y, sample_number, isect.p.x, isect.p.y, isect.p.z,
////                        default_string.c_str(), default_string.c_str(), default_string.c_str(), default_string.c_str(),
////                        s1.ToString().c_str(), s2.ToString().c_str(), default_string.c_str(),
////                        s8.ToString().c_str(), s9.ToString().c_str(), remap);
////
////                flock(fd, LOCK_UN);
////                fclose(material_props);
//                }
//
//                if (dynamic_cast<const PlasticMaterial *>(m)) {
//                    const PlasticMaterial *pm = dynamic_cast<const PlasticMaterial *>(m);
//                    //std::shared_ptr<Texture<Spectrum>> Kd = um->Kd;
//
//                    Spectrum s1 = pm->Kd->Evaluate(isect);
//                    Spectrum s2 = pm->Ks->Evaluate(isect);
//                    Spectrum s3 = pm->roughness->Evaluate(isect);
//                    std::string default_string = " [ 0.0, 0.0, 0.0 ]";
//                    float remap = pm->remapRoughness;
////
////                FILE *material_props = fopen("./materials_props.txt", "a");
////                int fd = fileno(material_props);
////                flock(fd, LOCK_EX);
////
////                fprintf(material_props, "%d, %d, %" PRId64 ", %f, %f, %f, %s, %s, %s, "
////                                        "%s, %s, %s, %s, %s, %s, %f\n",
////                        x, y, sample_number, isect.p.x, isect.p.y, isect.p.z, s1.ToString().c_str(),
////                        s2.ToString().c_str(), default_string.c_str(), default_string.c_str(),
////                        s3.ToString().c_str(), default_string.c_str(), default_string.c_str()
////                        , default_string.c_str(), default_string.c_str(), remap);
////
////                flock(fd, LOCK_UN);
////                fclose(material_props);
//                }
//
//                if (dynamic_cast<const MatteMaterial *>(m)) {
//                    const MatteMaterial *mm = dynamic_cast<const MatteMaterial *>(m);
//
//                    Spectrum s1 = mm->Kd->Evaluate(isect);
//                    std::string default_string = " [ 0.0, 0.0, 0.0 ]";
//                    float remap = 0.0f;
////
////                FILE *material_props = fopen("./materials_props.txt", "a");
////                int fd = fileno(material_props);
////                flock(fd, LOCK_EX);
////                fprintf(material_props, "%d, %d, %" PRId64 ", %f, %f, %f, %s, %s, %s, "
////                                        "%s, %s, %s, %s, %s, %s, %f\n",
////                        x, y, sample_number, isect.p.x, isect.p.y, isect.p.z, s1.ToString().c_str(),
////                        default_string.c_str(), default_string.c_str(), default_string.c_str(),
////                        default_string.c_str(), default_string.c_str(), default_string.c_str(),
////                        default_string.c_str(), default_string.c_str(), remap);
////
////                flock(fd, LOCK_UN);
////                fclose(material_props);
//                }
//
//                if (dynamic_cast<const MirrorMaterial *>(m)) {
//                    const MirrorMaterial *mm = dynamic_cast<const MirrorMaterial *>(m);
//                    //std::shared_ptr<Texture<Spectrum>> Kd = um->Kd;
//
//                    Spectrum s1 = mm->Kr->Evaluate(isect);
//                    std::string default_string = " [ 0.0, 0.0, 0.0 ]";
//                    float remap = 0.0f;
//
////                FILE *material_props = fopen("./materials_props.txt", "a");
////                int fd = fileno(material_props);
////
////                flock(fd, LOCK_EX);
////
////                fprintf(material_props, "%d, %d, %" PRId64 ", %f, %f, %f, %s, %s, %s, "
////                                        "%s, %s, %s, %s, %s, %s, %f\n",
////                        x, y, sample_number, isect.p.x, isect.p.y, isect.p.z, default_string.c_str(),
////                        default_string.c_str(), s1.ToString().c_str(), default_string.c_str(),
////                        default_string.c_str(), default_string.c_str(), default_string.c_str()
////                        , default_string.c_str(), default_string.c_str(), remap);
////
////                flock(fd, LOCK_UN);
////                fclose(material_props);
//                }
//
//                if (dynamic_cast<const SubstrateMaterial *>(m)) {
//                    const SubstrateMaterial *sm = dynamic_cast<const SubstrateMaterial *>(m);
//                    //std::shared_ptr<Texture<Spectrum>> Kd = um->Kd;
//
//                    Spectrum s1 = sm->Kd->Evaluate(isect);
//                    Spectrum s2 = sm->Ks->Evaluate(isect);
//
//                    Spectrum s8, s9;
//                    float remap = sm->remapRoughness;
//
//                    if(sm->nu == nullptr)
//                    {
//                        s8 = Spectrum(0.0f);
//                    }
//                    else
//                    {
//                        s8 = sm->nu->Evaluate(isect);
//                    }
//                    if(sm->nv == nullptr)
//                    {
//                        s9 = Spectrum(0.0f);
//                    }
//                    else
//                    {
//                        s9 = sm->nv->Evaluate(isect);
//                    }
//
//
//                    std::string default_string = " [ 0.0, 0.0, 0.0 ]";
////
////                FILE *material_props = fopen("./materials_props.txt", "a");
////                int fd = fileno(material_props);
////                flock(fd, LOCK_EX);
////                fprintf(material_props, "%d, %d, %" PRId64 ", %f, %f, %f, %s, %s, %s, "
////                                        "%s, %s, %s, %s, %s, %s, %f\n",
////                        x, y, sample_number, isect.p.x, isect.p.y, isect.p.z, s1.ToString().c_str(),
////                        s2.ToString().c_str(), default_string.c_str(), default_string.c_str(),
////                        default_string.c_str(), default_string.c_str(), default_string.c_str()
////                        , s8.ToString().c_str(), s9.ToString().c_str(), remap);
////
////                flock(fd, LOCK_UN);
////                fclose(material_props);
//                }
//            }


            // Possibly add emitted light at intersection
            if (bounces == 0 || specularBounce) {
                // Add emitted light at path vertex or from the environment
                if (foundIntersection) {
                    L += beta * isect.Le(-ray.d);
                    VLOG(2) << "Added Le -> L = " << L;
                } else
                {
                    if(bounces == 0 && !specularBounce && greenRender )
                    {
                        //Added
                        Float rgb[3];
                        rgb[0] = 0.0f;
                        rgb[1] = 0.15f;
                        rgb[2] = 0.0f;
                        Spectrum sss = Spectrum::FromRGB(rgb);
                        L += sss;
                    }
                    else
                    {
                        for (const auto &light : scene.infiniteLights)
                            L += beta * light->Le(ray);
                    }
                    VLOG(2) << "Added infinite area lights -> L = " << L;
                }
            }

            // Terminate path if ray escaped or _maxDepth_ was reached
            if (!foundIntersection || bounces >= maxDepth) break;

            // Compute scattering functions and skip over medium boundaries
            isect.ComputeScatteringFunctions(ray, arena, true);
            if (!isect.bsdf) {
                VLOG(2) << "Skipping intersection due to null bsdf";
                ray = isect.SpawnRay(ray.d);
                bounces--;
                continue;
            }

            const Distribution1D *distrib = lightDistribution->Lookup(isect.p);

            // Sample illumination from lights to find path contribution.
            // (But skip this for perfectly specular BSDFs.)
            if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~BSDF_SPECULAR)) >
                0) {
                ++totalPaths;
                Spectrum Ld = beta * UniformSampleOneLight(isect, scene, arena,
                                                           sampler, false, distrib);
                VLOG(2) << "Sampled direct lighting Ld = " << Ld;
                if (Ld.IsBlack()) ++zeroRadiancePaths;
                CHECK_GE(Ld.y(), 0.f);
                L += Ld;
            }

            // Sample BSDF to get new path direction
            Vector3f wo = -ray.d, wi;
            Float pdf;
            BxDFType flags;
            Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf,
                                              BSDF_ALL, &flags);
            VLOG(2) << "Sampled BSDF, f = " << f << ", pdf = " << pdf;
            if (f.IsBlack() || pdf == 0.f) break;
            beta *= f * AbsDot(wi, isect.shading.n) / pdf;
            VLOG(2) << "Updated beta = " << beta;
            CHECK_GE(beta.y(), 0.f);
            DCHECK(!std::isinf(beta.y()));
            specularBounce = (flags & BSDF_SPECULAR) != 0;
            if ((flags & BSDF_SPECULAR) && (flags & BSDF_TRANSMISSION)) {
                Float eta = isect.bsdf->eta;
                // Update the term that tracks radiance scaling for refraction
                // depending on whether the ray is entering or leaving the
                // medium.
                etaScale *= (Dot(wo, isect.n) > 0) ? (eta * eta) : 1 / (eta * eta);
            }
            ray = isect.SpawnRay(wi);

            // Account for subsurface scattering, if applicable
            if (isect.bssrdf && (flags & BSDF_TRANSMISSION)) {
                // Importance sample the BSSRDF
                SurfaceInteraction pi;
                Spectrum S = isect.bssrdf->Sample_S(
                        scene, sampler.Get1D(), sampler.Get2D(), arena, &pi, &pdf);
                DCHECK(!std::isinf(beta.y()));
                if (S.IsBlack() || pdf == 0) break;
                beta *= S / pdf;

                // Account for the direct subsurface scattering component
                L += beta * UniformSampleOneLight(pi, scene, arena, sampler, false,
                                                  lightDistribution->Lookup(pi.p));

                // Account for the indirect subsurface scattering component
                Spectrum f = pi.bsdf->Sample_f(pi.wo, &wi, sampler.Get2D(), &pdf,
                                               BSDF_ALL, &flags);
                if (f.IsBlack() || pdf == 0) break;
                beta *= f * AbsDot(wi, pi.shading.n) / pdf;
                DCHECK(!std::isinf(beta.y()));
                specularBounce = (flags & BSDF_SPECULAR) != 0;
                ray = pi.SpawnRay(wi);
            }

            // Possibly terminate the path with Russian roulette.
            // Factor out radiance scaling due to refraction in rrBeta.
            Spectrum rrBeta = beta * etaScale;
            if (rrBeta.MaxComponentValue() < rrThreshold && bounces > 3) {
                Float q = std::max((Float).05, 1 - rrBeta.MaxComponentValue());
                if (sampler.Get1D() < q) break;
                beta /= 1 - q;
                DCHECK(!std::isinf(beta.y()));
            }
        }

        ReportValue(pathLength, bounces);
        return L;
    }

    PathIntegrator *CreatePathIntegrator(const ParamSet &params,
                                         std::shared_ptr<Sampler> sampler,
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
        Float rrThreshold = params.FindOneFloat("rrthreshold", 1.);
        std::string lightStrategy =
                params.FindOneString("lightsamplestrategy", "spatial");
        return new PathIntegrator(maxDepth, camera, sampler, pixelBounds,
                                  rrThreshold, lightStrategy);
    }

}  // namespace pbrt
