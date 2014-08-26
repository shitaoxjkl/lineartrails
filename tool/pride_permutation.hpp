//-----------------------------------------------------------------------------

template <unsigned rounds>
PridePermutation<rounds>::PridePermutation() {
  for(int i = 0; i< 2*rounds +1; ++i){
      this->state_masks_[i].reset(new PrideState);
    }
  for (int i = 0; i < rounds; ++i) {
    this->sbox_layers_[i].reset(new PrideSboxLayer);
    this->sbox_layers_[i]->SetMasks(this->state_masks_[2*i].get(), this->state_masks_[2*i + 1].get());
    this->linear_layers_[i].reset(new PrideLinearLayer);
    this->linear_layers_[i]->SetMasks(this->state_masks_[2*i + 1].get(), this->state_masks_[2*i + 2].get());
  }
  touchall();
}

template <unsigned rounds>
PridePermutation<rounds>::PridePermutation(const PridePermutation& other) : Permutation<rounds>(other) {
}

template <unsigned rounds>
PridePermutation<rounds>& PridePermutation<rounds>::operator=(const PridePermutation<rounds>& rhs){
  for(int i = 0; i< 2*rounds +1; ++i){
    this->state_masks_[i].reset(rhs.state_masks_[i]->clone());
  }
  this->toupdate_linear = rhs.toupdate_linear;
 this->toupdate_nonlinear = rhs.toupdate_nonlinear;
 for (int i = 0; i < rounds; ++i) {
   this->sbox_layers_[i].reset(rhs.sbox_layers_[i]->clone());
   this->sbox_layers_[i]->SetMasks(this->state_masks_[2*i].get(), this->state_masks_[2*i + 1].get());
   this->linear_layers_[i].reset(rhs.linear_layers_[i]->clone());
   this->linear_layers_[i]->SetMasks(this->state_masks_[2*i + 1].get(), this->state_masks_[2*i + 2].get());
 }
 return *this;
}

template <unsigned rounds>
PridePermutation<rounds>* PridePermutation<rounds>::clone() const{
  return new PridePermutation(*this);
}

template<unsigned rounds>
void PridePermutation<rounds>::PrintWithProbability(std::ostream& stream, int offset) {
  Permutation<rounds>::PrintWithProbability(stream, 0);
}

template <unsigned rounds>
void PridePermutation<rounds>::touchall() {
  Permutation<rounds>::touchall();
}

template <unsigned rounds>
bool PridePermutation<rounds>::setBit(BitMask cond, unsigned int bit){
  return Permutation<rounds>::setBit(cond,bit,8,8);
}


