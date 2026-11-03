import { Component, EventEmitter, OnInit, Output } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { AuthService } from '../../services/auth.service';
import { AuthResult } from '../../models/enums';

@Component({
  selector: 'app-auth',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './auth.component.html',
  styleUrl: './auth.component.css'
})
export class AuthComponent implements OnInit {
  @Output()
  result = new EventEmitter<AuthResult>();
  password = '';
  remember = false;
  error = '';
  loading = false;

  constructor(private authService: AuthService) {}

  ngOnInit(): void {
    this.authService.autoLogin().then(success => {
      if (success) {
        this.result.emit(AuthResult.Success);
      }
    });
  }

  onKeydown(event: KeyboardEvent): void {
    if (event.key === 'Enter' && this.password.trim().length) {
      this.login();
    }
  }

  login(): void {
    if (!this.password.trim().length) return;
    this.loading = true;
    this.error = '';
    this.authService.login(this.password, this.remember).then(success => {
      this.loading = false;
      if (success) {
        this.result.emit(AuthResult.Success);
      } else {
        this.error = 'Invalid password or server error';
        this.result.emit(AuthResult.Fail);
      }
    });
  }
}