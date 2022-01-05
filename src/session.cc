/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2021 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 #include <udjat/tools/usersession.h>
 #include <systemd/sd-login.h>
 #include <iostream>

 using namespace std;

 namespace Udjat {

	User::Session::Session() {
	}

	User::Session::~Session() {
	}

	User::Session & User::Session::onEvent(const User::Event &event) noexcept {
#ifdef DEBUG
		cout << "session\t sid=" << this->sid << " Event=" << (int) event
				<< " Alive=" << (alive() ? "Yes" : "No")
				<< " Remote=" << (remote() ? "Yes" : "No")
				<< endl;
#endif // DEBUG
		return *this;
	}

	bool User::Session::remote() const {
		// https://www.carta.tech/man-pages/man3/sd_session_is_remote.3.html
		return (sd_session_is_remote(sid.c_str()) > 0);
	}

 }

