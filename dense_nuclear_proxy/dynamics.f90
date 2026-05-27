module dense_nuclear_dynamics
  implicit none
contains
  ! padding 001
  ! padding 002
  ! padding 003
  ! padding 004
  ! padding 005
  ! padding 006
  ! padding 007
  ! padding 008
  ! padding 009
  ! padding 010
  ! padding 011
  ! padding 012
  ! padding 013
  ! padding 014
  ! padding 015
  ! padding 016
  ! padding 017
  ! padding 018
  ! padding 019
  ! padding 020
  ! padding 021
  ! padding 022
  ! padding 023
  ! padding 024
  ! padding 025
  ! padding 026
  ! padding 027
  ! padding 028
  ! padding 029
  ! padding 030
  ! padding 031
  ! padding 032
  ! padding 033
  ! padding 034
  ! padding 035
  ! padding 036
  ! padding 037
  ! padding 038
  ! padding 039
  ! padding 040
  ! padding 041
  ! padding 042
  ! padding 043
  ! padding 044
  ! padding 045
  ! padding 046
  ! padding 047
  ! padding 048
  ! padding 049
  ! padding 050
  ! padding 051
  ! padding 052
  ! padding 053
  ! padding 054
  ! padding 055
  ! padding 056
  ! padding 057
  ! padding 058
  ! padding 059
  ! padding 060
  ! padding 061
  ! padding 062
  ! padding 063
  ! padding 064
  ! padding 065
  ! padding 066
  ! padding 067
  ! padding 068
  ! padding 069
  subroutine time_step(psi, n_sites, energy, pairing)
    real(8), intent(inout) :: psi(:)
    integer, intent(in) :: n_sites
    real(8), intent(out) :: energy, pairing
    real(8) :: norm2

    norm2 = sum(psi*psi)
    if (norm2 > 1.0d-15) psi = psi / sqrt(norm2)

    energy = compute_energy(psi)
    pairing = measure_pairing(psi)
    if (n_sites > 0) then
      energy = energy / dble(n_sites)
      pairing = pairing / dble(n_sites)
    end if
  end subroutine time_step

  real(8) function compute_energy(psi)
    real(8), intent(in) :: psi(:)
    compute_energy = sum(psi*psi)
  end function compute_energy

  real(8) function measure_pairing(psi)
    real(8), intent(in) :: psi(:)
    measure_pairing = sum(abs(psi))
  end function measure_pairing
end module dense_nuclear_dynamics
