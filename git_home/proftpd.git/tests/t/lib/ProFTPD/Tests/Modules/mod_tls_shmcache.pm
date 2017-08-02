package ProFTPD::Tests::Modules::mod_tls_shmcache;

use lib qw(t/lib);
use base qw(ProFTPD::TestSuite::Child);
use strict;

use File::Spec;
use IO::Handle;
use IPC::Open3;

use ProFTPD::TestSuite::FTP;
use ProFTPD::TestSuite::Utils qw(:auth :config :running :test :testsuite);

$| = 1;

my $order = 0;

my $TESTS = {
  tls_sess_cache_shm => {
    order => ++$order,
    test_class => [qw(forking)],
  },

};

sub new {
  return shift()->SUPER::new(@_);
}

sub list_tests {
  # Check for the required Perl modules:
  #
  #  Net-SSLeay
  #  IO-Socket-SSL
  #  Net-FTPSSL

  my $required = [qw(
    Net::SSLeay
    IO::Socket::SSL
    Net::FTPSSL
  )];

  foreach my $req (@$required) {
    eval "use $req";
    if ($@) {
      print STDERR "\nWARNING:\n + Module '$req' not found, skipping all tests\n";

      if ($ENV{TEST_VERBOSE}) {
        print STDERR "Unable to load $req: $@\n";
      }

      return qw(testsuite_empty_test);
    }
  }

  return testsuite_get_runnable_tests($TESTS);
}

sub tls_sess_cache_shm {
  my $self = shift;
  my $tmpdir = $self->{tmpdir};

  my $config_file = "$tmpdir/tls.conf";
  my $pid_file = File::Spec->rel2abs("$tmpdir/tls.pid");
  my $scoreboard_file = File::Spec->rel2abs("$tmpdir/tls.scoreboard");

  my $log_file = test_get_logfile();

  my $auth_user_file = File::Spec->rel2abs("$tmpdir/tls.passwd");
  my $auth_group_file = File::Spec->rel2abs("$tmpdir/tls.group");

  my $user = 'proftpd';
  my $passwd = 'test';
  my $group = 'ftpd';
  my $home_dir = File::Spec->rel2abs($tmpdir);
  my $uid = 500;
  my $gid = 500;

  # Make sure that, if we're running as root, that the home directory has
  # permissions/privs set for the account we create
  if ($< == 0) {
    unless (chmod(0755, $home_dir)) {
      die("Can't set perms on $home_dir to 0755: $!");
    }

    unless (chown($uid, $gid, $home_dir)) {
      die("Can't set owner of $home_dir to $uid/$gid: $!");
    }
  }

  auth_user_write($auth_user_file, $user, $passwd, $uid, $gid, $home_dir,
    '/bin/bash');
  auth_group_write($auth_group_file, $group, $gid, $user);

  my $cert_file = File::Spec->rel2abs('t/etc/modules/mod_tls/server-cert.pem');
  my $ca_file = File::Spec->rel2abs('t/etc/modules/mod_tls/ca-cert.pem');

  my $shm_path = File::Spec->rel2abs("$tmpdir/tls-shmcache");
  my $sessid_file = File::Spec->rel2abs("$tmpdir/sessid.pem");

  my $config = {
    PidFile => $pid_file,
    ScoreboardFile => $scoreboard_file,
    SystemLog => $log_file,
    TraceLog => $log_file,
    Trace => 'tls_shmcache:20',

    AuthUserFile => $auth_user_file,
    AuthGroupFile => $auth_group_file,

    IfModules => {
      'mod_delay.c' => {
        DelayEngine => 'off',
      },

      'mod_tls.c' => {
        TLSEngine => 'on',
        TLSLog => $log_file,
        TLSProtocol => 'SSLv3 TLSv1',
        TLSRequired => 'on',
        TLSRSACertificateFile => $cert_file,
        TLSCACertificateFile => $ca_file,
        TLSVerifyClient => 'off',
      },

      'mod_tls_shmcache.c' => {
        # 10332 is the minimum number of bytes for shmcache
        TLSSessionCache => "shm:/file=$shm_path&size=20664",
      },
    },
  };

  my ($port, $config_user, $config_group) = config_write($config_file, $config);

  # Open pipes, for use between the parent and child processes.  Specifically,
  # the child will indicate when it's done with its test by writing a message
  # to the parent.
  my ($rfh, $wfh);
  unless (pipe($rfh, $wfh)) {
    die("Can't open pipe: $!");
  }

  my $ex;

  # Fork child
  $self->handle_sigchld();
  defined(my $pid = fork()) or die("Can't fork: $!");
  if ($pid) {
    eval {
      # Give the server a chance to start up
      sleep(2);

      # To test SSL session resumption, we use the command-line
      # openssl s_client tool, rather than any Perl module.

      # XXX Some OpenSSL versions' of s_client do not support the 'ftp'
      # parameter for -starttls; in this case, point the openssl binary
      # to be used to a version which does support this.
      my $openssl = 'openssl';

      my @cmd = (
        $openssl,
        's_client',
        '-connect',
        "127.0.0.1:$port",
        '-starttls',
        'ftp',
        '-sess_out',
        $sessid_file,
        '-CAfile',
        $ca_file,
      );

      my $tls_rh = IO::Handle->new();
      my $tls_wh = IO::Handle->new();
      my $tls_eh = IO::Handle->new();

      $tls_wh->autoflush(1);

      local $SIG{CHLD} = 'DEFAULT';

      if ($ENV{TEST_VERBOSE}) {
        print STDERR "Executing: ", join(' ', @cmd), "\n";
      }

      my $tls_pid = open3($tls_wh, $tls_rh, $tls_eh, @cmd);
      print $tls_wh "quit\n";
      waitpid($tls_pid, 0);

      my ($res, $cipher_str, $err_str, $out_str);
      if ($? >> 8) {
        $err_str = join('', <$tls_eh>);
        $res = 0;

      } else {
        my $output = [<$tls_rh>];

        # Specifically look for the line containing 'Cipher is'
        foreach my $line (@$output) {
          if ($line =~ /Cipher is/) {
            $cipher_str = $line;
            chomp($cipher_str);
          }
        }

        if ($ENV{TEST_VERBOSE}) {
          $out_str = join('', @$output);
          print STDERR "Stdout: $out_str\n";
        }

        if ($ENV{TEST_VERBOSE}) {
          $err_str = join('', <$tls_eh>);
          print STDERR "Stderr: $err_str\n";
        }

        $res = 1;
      }

      unless ($res) {
        die("Can't talk to server: $err_str");
      }

      my $expected = '^New';
      $self->assert(qr/$expected/, $cipher_str,
        test_msg("Expected '$expected', got '$cipher_str'"));

      # Wait for a couple of seconds
      sleep(2);

      @cmd = (
        $openssl,
        's_client',
        '-connect',
        "127.0.0.1:$port",
        '-starttls',
        'ftp',
        '-sess_in',
        $sessid_file,
        '-CAfile',
        $ca_file,
      );

      $tls_rh = IO::Handle->new();
      $tls_wh = IO::Handle->new();
      $tls_eh = IO::Handle->new();

      $tls_wh->autoflush(1);

      if ($ENV{TEST_VERBOSE}) {
        print STDERR "Executing: ", join(' ', @cmd), "\n";
      }

      $tls_pid = open3($tls_wh, $tls_rh, $tls_eh, @cmd);
      print $tls_wh "quit\n";
      waitpid($tls_pid, 0);

      $res = 0;
      $cipher_str = undef;
      $err_str = undef;
      $out_str = undef;

      if ($? >> 8) {
        $err_str = join('', <$tls_eh>);
        $res = 0;

      } else {
        my $output = [<$tls_rh>];

        # Specifically look for the line containing 'Cipher is'
        foreach my $line (@$output) {
          if ($line =~ /Cipher is/) {
            $cipher_str = $line;
            chomp($cipher_str);
          }
        }

        if ($ENV{TEST_VERBOSE}) {
          $out_str = join('', @$output);
          print STDERR "Stdout: $out_str\n";
        }

        if ($ENV{TEST_VERBOSE}) {
          $err_str = join('', <$tls_eh>);
          print STDERR "Stderr: $err_str\n";
        }

        $res = 1;
      }

      unless ($res) {
        die("Can't talk to server: $err_str");
      }

      $expected = '^Reused';
      $self->assert(qr/$expected/, $cipher_str,
        test_msg("Expected '$expected', got '$cipher_str'"));
    };

    if ($@) {
      $ex = $@;
    }

    $wfh->print("done\n");
    $wfh->flush();

  } else {
    eval { server_wait($config_file, $rfh, 45) };
    if ($@) {
      warn($@);
      exit 1;
    }

    exit 0;
  }

  # Stop server
  server_stop($pid_file);

  $self->assert_child_ok($pid);

  if ($ex) {
    test_append_logfile($log_file, $ex);
    unlink($log_file);

    die($ex);
  }

  unlink($log_file);
}

1;
