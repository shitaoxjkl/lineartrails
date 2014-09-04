#ifndef HAMSI_H_
#define HAMSI_H_

#include <vector>
#include <array>
#include <random>

#include "layer.h"
#include "mask.h"
#include "statemask.h"
#include "step_linear.h"
#include "step_nonlinear.h"
#include "updatequeue.h"
#include "memory"
#include "lrucache.h"


struct HamsiState : public StateMask<16,32> {
  HamsiState();
  friend std::ostream& operator<<(std::ostream& stream, const HamsiState& statemask);
  void print(std::ostream& stream);
  virtual HamsiState* clone();
};


#define ROTL32(x,n) (((x)<<(n))|((x)>>(32-(n))))



struct HamsiLinearLayer : public LinearLayer {
  HamsiLinearLayer& operator=(const HamsiLinearLayer& rhs);
  HamsiLinearLayer();
  virtual HamsiLinearLayer* clone();
  void Init();
  HamsiLinearLayer(StateMaskBase *in, StateMaskBase *out);
  virtual bool Update(UpdatePos pos);
  int GetNumLayer();

  std::array<LinearStep<32, 4>, 4> layers;
  static std::unique_ptr<LRU_Cache<WordMaskArray<32, 4>, LinearStepUpdateInfo<32, 4>>> cache_[1];
};


struct HamsiSboxLayer : public SboxLayer<4, 128> {
  HamsiSboxLayer& operator=(const HamsiSboxLayer& rhs);
  HamsiSboxLayer();
  HamsiSboxLayer(StateMaskBase *in, StateMaskBase *out);
  virtual HamsiSboxLayer* clone();
  void InitSboxes();
  virtual bool Update(UpdatePos pos);
  Mask GetVerticalMask(int b, const StateMaskBase& s) const;
  void SetVerticalMask(int b, StateMaskBase& s, const Mask& mask);

 static std::unique_ptr<LRU_Cache<unsigned long long,NonlinearStepUpdateInfo>> cache_;
};



#endif // HAMSI_H_
