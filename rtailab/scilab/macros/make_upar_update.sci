// COPYRIGHT (C) 2002  Lorenzo Dozio (dozio@aero.polimi.it)
//                     Paolo Mantegazza (mantegazza@aero.polimi.it)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.

function Code=make_upar_update()
//callback function at user parameters updates on the fly

  Code=['/*'+part('-',ones(1,40))+' callback at user params updates */ ';
	'void '
	cformatline(rdnom+'_upar_update(int index)',70);
	'{'
	'}'
	''];
endfunction
