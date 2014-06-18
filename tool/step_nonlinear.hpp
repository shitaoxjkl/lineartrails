#include "mask.h"

template <unsigned bitsize>
LinearDistributionTable<bitsize>::LinearDistributionTable(std::function<BitVector(BitVector)> fun) {
  Initialize(fun);
}

template <unsigned bitsize>
void LinearDistributionTable<bitsize>::Initialize(const std::function<BitVector(BitVector)> fun) {
  unsigned int boxsize = (1 << (bitsize));

  //resize vector
  ldt.resize(boxsize, std::vector<signed>(boxsize));
  ldt_bool.resize(boxsize, std::vector<unsigned>(boxsize));

  //calc ldt
  for (unsigned int a = 0; a < boxsize; a++)
    for (unsigned int b = 0; b < boxsize; b++)
      ldt[a][b] = -(boxsize >> 1);

  for (unsigned int x = 0; x < boxsize; x++) {
    for (unsigned int a = 0; a < boxsize; ++a) {
      unsigned int facta = a & x;
      unsigned int asum = 0;
      for (unsigned int j = 0; j < bitsize; ++j)
        asum ^= ((facta >> j) & 1);
      for (unsigned int b = 0; b < boxsize; ++b) {
        unsigned int factb = b & fun(x);
        unsigned int bsum = 0;
        for (unsigned int j = 0; j < bitsize; ++j)
          bsum ^= ((factb >> j) & 1);
        ldt[a][b] += (asum == bsum);
      }
    }
  }

  for (unsigned int a = 0; a < boxsize; ++a)
    for (unsigned int b = 0; b < boxsize; ++b)
      if (ldt[a][b] != 0)
        ldt_bool[a][b] = ~0U;
}

template <unsigned bitsize>
LinearDistributionTable<bitsize>& LinearDistributionTable<bitsize>::operator=(const LinearDistributionTable<bitsize>& rhs){
  ldt_bool = rhs.ldt_bool;
  ldt = rhs.ldt;
  return *this;
}

template <unsigned bitsize>
std::ostream& operator<<(std::ostream& stream, const LinearDistributionTable<bitsize>& ldt) {
  // TODO
  return stream;
}

//-----------------------------------------------------------------------------

template <unsigned bitsize>
NonlinearStep<bitsize>::NonlinearStep(std::function<BitVector(BitVector)> fun) {
  Initialize(fun);
}

template <unsigned bitsize>
void NonlinearStep<bitsize>::Initialize(std::function<BitVector(BitVector)> fun) {
  is_active_ = false;
  is_guessable_ = true;
  ldt_.reset(new LinearDistributionTable<bitsize>(fun));
}

template <unsigned bitsize>
void NonlinearStep<bitsize>::Initialize(std::shared_ptr<LinearDistributionTable<bitsize>> ldt) {
  is_active_ = false;
  is_guessable_ = true;
  ldt_ = ldt;
}

template <unsigned bitsize>
bool NonlinearStep<bitsize>::Update(Mask& x, Mask& y) {
  std::vector<unsigned int> inmasks, outmasks;  // TODO check datatype!
  unsigned int inresult[2] = { 0, 0 };  // TODO check datatype!
  unsigned int outresult[2] = { 0, 0 };
  create_masks(inmasks, x);
  create_masks(outmasks, y);
  for (const auto& inmask : inmasks)
    for (const auto& outmask : outmasks) {
      inresult[0] |= (~inmask) & ldt_->ldt_bool[inmask][outmask];
      inresult[1] |= inmask & ldt_->ldt_bool[inmask][outmask];
      outresult[0] |= (~outmask) & ldt_->ldt_bool[inmask][outmask];
      outresult[1] |= outmask & ldt_->ldt_bool[inmask][outmask];
    }

  for (unsigned int i = 0; i < bitsize; ++i) {
    x.bitmasks[i] = ((inresult[1] & (1 << i))
        | ((inresult[0] & (1 << i)) << 1)) >> i;
    y.bitmasks[i] = ((outresult[1] & (1 << i))
        | ((outresult[0] & (1 << i)) << 1)) >> i;
  }

  x.reinit_caremask();
  y.reinit_caremask();

  if ((inresult[0] | inresult[1]) == 0 || (outresult[0] | outresult[1]) == 0)
    return false;

  if ( (((~x.caremask.canbe1) | (~x.caremask.care)) & (~0ULL >> (64 - bitsize)))  == (~0ULL >> (64 - bitsize))
      && (((~y.caremask.canbe1) | (~y.caremask.care)) & (~0ULL >> (64 - bitsize)))  == (~0ULL >> (64 - bitsize)))
    is_active_ = false;
  else
    is_active_ = true;

  is_guessable_ = false;
  for(unsigned int i = 0; i < bitsize; ++i)
    if(x.bitmasks[i] == BM_DUNNO || y.bitmasks[i] == BM_DUNNO){
      is_guessable_ = true;
      break;
    }


  return true;
}

template<unsigned bitsize>
bool NonlinearStep<bitsize>::Update(
    Mask& x, Mask& y,
    Cache<unsigned long long, NonlinearStepUpdateInfo>* box_cache) {
  NonlinearStepUpdateInfo stepdata;
  x.reinit_caremask();
  y.reinit_caremask();
  unsigned long long key = getKey(x, y);

  if (box_cache->find(key, stepdata)) {
    is_active_ = stepdata.is_active_;
    is_guessable_ = stepdata.is_guessable_;
    x.bitmasks = stepdata.inmask_;
    y.bitmasks = stepdata.outmask_;
    x.reinit_caremask();
    y.reinit_caremask();
    return true;
  }

  if (Update(x, y)) {
    stepdata.is_active_ = is_active_;
    stepdata.is_guessable_ = is_guessable_;
    stepdata.inmask_ = x.bitmasks;
    stepdata.outmask_ = y.bitmasks;
    x.reinit_caremask();
    y.reinit_caremask();
    box_cache->insert(key, stepdata);
    return true;
  }

  return false;

}

template<unsigned bitsize>
unsigned long long NonlinearStep<bitsize>::getKey(Mask& in, Mask& out) {
  return ((in.caremask.canbe1) << (3 * bitsize))
      | ((in.caremask.care) << (2 * bitsize))
      | ((out.caremask.canbe1) << bitsize)
      | ((out.caremask.care));
}

template <unsigned bitsize>
ProbabilityPair NonlinearStep<bitsize>::GetProbability(Mask& x, Mask& y) {
  std::vector<unsigned int> inmasks, outmasks;
  create_masks(inmasks, x);
  create_masks(outmasks, y);

  //TODO: Maybe calculate some probability if undefined bits are present
  if(inmasks.size() > 1)
    return ProbabilityPair {1,-1};
 if(ldt_->ldt[inmasks[0]][outmasks[0]] == 0)
    return ProbabilityPair {1,-1};
  return ProbabilityPair {(char)(ldt_->ldt[inmasks[0]][outmasks[0]]
      / std::abs(ldt_->ldt[inmasks[0]][outmasks[0]])), (double) (std::log2((double)std::abs(
      ldt_->ldt[inmasks[0]][outmasks[0]])) - bitsize)};

}

template <unsigned bitsize>
NonlinearStep<bitsize>& NonlinearStep<bitsize>::operator=(const NonlinearStep<bitsize>& rhs){
  ldt_ = rhs.ldt_;
  is_active_ = rhs.is_active_;
  return *this;
}

template <unsigned bitsize>
void NonlinearStep<bitsize>::TakeBestBox(Mask& x, Mask& y) {
  std::vector<unsigned int> inmasks, outmasks;  // TODO check datatype!
  create_masks(inmasks, x);
  create_masks(outmasks, y);
    int branch_number = 0;
    unsigned int best_inmask = 0;
    unsigned int best_outmask = 0;

  for (const auto& inmask : inmasks)
    for (const auto& outmask : outmasks) {
       if(branch_number < std::abs(ldt_->ldt[inmask][outmask])){
         branch_number = std::abs(ldt_->ldt[inmask][outmask]);
         best_inmask = inmask;
         best_outmask = outmask;
       }
    }

  for (unsigned int i = 0; i < bitsize; ++i) {
    x.bitmasks[i] = (((best_inmask  >> i)&1) == 1 ? BM_1 : BM_0);
    y.bitmasks[i] = (((best_outmask  >> i)&1) == 1 ? BM_1 : BM_0);
  }

  if(best_inmask)
    is_active_ = true;
  else
    is_active_ = false;

  is_guessable_ = false;

  x.reinit_caremask();
  y.reinit_caremask();

}

template <unsigned bitsize>
int NonlinearStep<bitsize>::TakeBestBox(Mask& x, Mask& y, int pos) {
  std::vector<unsigned int> inmasks, outmasks;  // TODO check datatype!
  create_masks(inmasks, x);
  create_masks(outmasks, y);
  std::multimap<int,std::pair<unsigned int,unsigned int>,std::greater<int>> valid_masks;


  for (const auto& inmask : inmasks)
    for (const auto& outmask : outmasks) {
       if(0 < std::abs(ldt_->ldt[inmask][outmask])){
         valid_masks.insert(std::pair<int,std::pair<unsigned int,unsigned int>>(std::abs(ldt_->ldt[inmask][outmask]),std::pair<unsigned int, unsigned int>(inmask,outmask)));
       }
    }

  assert(pos<valid_masks.size());

  for (unsigned int i = 0; i < bitsize; ++i) {
    x.bitmasks[i] = (((std::next(valid_masks.begin(),pos)->second.first  >> i)&1) == 1 ? BM_1 : BM_0);
    y.bitmasks[i] = (((std::next(valid_masks.begin(),pos)->second.second  >> i)&1) == 1 ? BM_1 : BM_0);
  }

  if(std::next(valid_masks.begin(),pos)->second.first)
    is_active_ = true;
  else
    is_active_ = false;

  is_guessable_ = false;

  x.reinit_caremask();
  y.reinit_caremask();

  return valid_masks.size();

}

template <unsigned bitsize>
void NonlinearStep<bitsize>::create_masks(std::vector<unsigned int> &masks,
                                          Mask& reference, unsigned int pos,
                                          unsigned int current_mask) {
  if (pos < bitsize) {
    switch (reference.bitmasks[pos]) {
      case BM_1:
        current_mask |= (1 << pos);
        create_masks(masks, reference, ++pos, current_mask);
        break;
      case BM_0:
        create_masks(masks, reference, ++pos, current_mask);
        break;
      case BM_DUNNO:
        create_masks(masks, reference, ++pos, current_mask);
        current_mask |= (1 << ((--pos)));
        create_masks(masks, reference, ++pos, current_mask);
        break;
    }
  } else {
    masks.push_back(current_mask);
  }
}

template <unsigned bitsize>
std::ostream& operator<<(std::ostream& stream, const NonlinearStep<bitsize>& step) {
  // TODO
  return stream;
}

